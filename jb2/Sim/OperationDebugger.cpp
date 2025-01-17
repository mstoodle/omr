/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Debug/Debugger.hpp"
#include "Debug/DebugValue.hpp"
#include "Debug/OperationDebugger.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

OperationDebugger::OperationDebugger(LOCATION, Debugger *dbgr, Base::FunctionCompilation *comp, Operation *op)
    : DebuggerFunction(PASSLOC, dbgr, comp)
    , _dbgrName("debugger")
    , _localsName("locals")
    , _valuesName("values")
    , _frameName("frame")
    , _fromBuilderID("fromBuilderID")
    , _op(op) {

    Base::Function *func = comp->func();
    DefineName(func->name() + String(".op") + String::to_string(op->id()));
    DefineFile("OpDbgr");
    DefineLine(op->name());
}

bool
OperationDebugger::buildContext(LOCATION, Base::FunctionCompilation *comp, Base::FunctionContext *fc) {
    this->DebuggerFunction::buildContext(PASSLOC, comp, fc);
    _dbgrSym = fc->DefineLocal(_dbgrName, dbgDict()->_pDebugValue);
    _localsSym = fc->DefineLocal(_localsName, dbgDict()->_pDebugValue);
    _valuesSym = fc->DefineLocal(_valuesName, dbgDict()->_pDebugValue);
    _frameSym = fc->DefineParameter(_frameName, dbgDict()->_pDebugFrame);
    _fromBuilderIDSym = fc->DefineParameter(_fromBuilderID, _base->Int32);
    fc->DefineReturnType(_base->Int8);
    return true;
}

bool
OperationDebugger::debug(DebuggerFrame *frame, Operation *op) {
    return frame->_info->_operationDebuggers[op->id()](frame, frame->_fromBuilder->id());
}

void
OperationDebugger::setDebuggerBuilderTarget(Builder *b, Builder *targetBuilder) {
}

void
OperationDebugger::copyResult(DebugValue *dest, DebugValue *src) {
    memcpy(dest, src, dbgDict()->_DebugValue->size());
}

String
OperationDebugger::valueName(Value *v) {
    return String("#_v") + String::to_string(v->id());
}

// locals are stored in DebugValues in the debugger frame, but operations that access
// locals will do so by their name encoded in Symbol (Local, Parameter, Function).
// At entry, copy the debug values for any string typed Literal that match the
// names of symbols into actual locals for this function with the string name. That way,
// any operation that accesses the local value will be able to load it directly.
// Similarly, values (operands) are stored ni DebugValues in debugger frame. To ensure
// that any code path can access those values safely, we load these into local variables
// at the beginning of the function, That way, whereever the operands are used in the
// generated code for the operation, the operands can be loaded safely from these locals.
// Otherwise, there can be cases where the operand values are actually loaded only on
// some paths (like the entry path) but e.g. not when control is coming back from some
// bound builder.
void
OperationDebugger::handleLocalsAndValuesIncoming(Builder *b) {
    Base::FunctionContext *fc = _comp->funcContext();
    for (auto sIt = _op->SymbolsBegin(); sIt != _op->SymbolsEnd(); sIt++) {
        Symbol *sym = *sIt;
        _base->Store(LOC, b, sym, loadValue(LOC, fc, b, sym));
    }
}

// locals are stored directly in the frame, but other operations will need to access
// their values as DebugValues from the debugger frame. On any outgoing path, then,
// we store the values for any locals into their corresponding DebugValue in the frame.
// Must be called on any outgoing path from the operation, to ensure the local value is
// available to other operation debuggers.
void
OperationDebugger::handleLocalsOutgoing(Builder *b) {
    Base::FunctionContext *fc = _comp->funcContext();
    for (auto sIt = _op->SymbolsBegin(); sIt != _op->SymbolsEnd(); sIt++) {
        Symbol *sym = *sIt;
        storeValue(LOC, fc, b, sym, _base->Load(LOC, b, sym));
    }
}

bool
OperationDebugger::buildIL(LOCATION, Base::FunctionCompilation *comp, FunctionContext *fc) {
    Builder *entry = fc->builderEntryPoint();

    Value *frame = _base->Load(LOC, entry, _frameSym);
    _base->Store(LOC, entry, _dbgrSym, _base->LoadFieldAt(LOC, entry, _DebugFrame_debugger, frame));
    _base->Store(LOC, entry, _localsSym, _base->LoadFieldAt(LOC, entry, _DebugFrame_locals, frame));
    _base->Store(LOC, entry, _valuesSym, _base->LoadFieldAt(LOC, entry, _DebugFrame_values, frame));

    handleLocalsAndValuesIncoming(entry);

    OperationCloner cloner(_op);

    // convert operands v of this operation to load their values from the DebugValues in the frame
    // operands *must* be loaded here so they reach all paths
    if (_op->numOperands() > 0) {
        int32_t opNum=0;
        for (auto opIt = _op->OperandsBegin(); opIt != _op->OperandsEnd(); opIt++) {
            Value *origOperand = *opIt;
            cloner.changeOperand(loadValue(LOC, fc, entry, origOperand), opNum++);
           //cloner.changeOperand(firstEntry->Load(valueName(origOperand)), opNum++);
        }
    }


    // generate switch based on incoming builder id
    //    each case jumps to the END of the corresponding builder used in the operation (builder B for each builder operand)
    // generate the operation Op_dbg, but the builder operands:
    //    contain two builders, A and B
    //        A contains the code to redirect the debugger control to the builder operand of the actual operation being debugged (passed as a parameter)
    //        B is just a label builder that is appended to the end of A. It is used to transfer control back from the corresponding builder of the
    //            operation being debugged via the switch above
    // Operation class must be able to generate Op_dbg via a virtual call
    //   createActionDebugger()
 
    // allocate builder objects to handle debugger transition to each of the
    //  the builders referenced by this operation
    // Each builder writes the target builder (from the original operation) into the
    //  debugger object and then returns true to the debugger, indicating that this handler
    //  has been "suspended" so that control can flow to debugger->_builderToDebug

    Builder *orphanTargets = NULL;
    int32_t numBoundBuilders = 0;
    if (_op->numBuilders() > 0) {
        orphanTargets = _base->OrphanBuilder(LOC, _op->parent());
        for (int32_t bIdx = 0; bIdx < _op->numBuilders(); bIdx++) {
            Builder *op_b = _op->builder(bIdx);
            Builder *b = _base->OrphanBuilder(LOC, op_b->parent());
            cloner.changeBuilder(b, bIdx);
            handleLocalsOutgoing(b);
            _base->StoreFieldAt(LOC, b, _DebugFrame_builderToDebug, _base->Load(LOC, b, _frameSym), _base->ConstAddress(LOC, b, (void *)op_b));
            _base->Return(LOC, b, _base->ConstInt8(LOC, b, 1));
         

            if (op_b->isBound() && _op == op_b->boundToOperation())
                numBoundBuilders++;
            else
                orphanTargets->AppendBuilder(b);
        }
    }

    // no need to change Types or Literal because the action debugger borrowed Types from original's
    // TypeDictionary and Literal don't require translation

    // need to change Symbols from those in the original FunctionBuiilder to those in this OperationDebugger
    if (_op->numSymbols() > 0) {
        for (auto s=0;s < _op->numSymbols();s++) {
            Symbol *origSymbol = _op->symbol(s);
            Symbol *debugOpSymbol = fc->getSymbol(origSymbol->name());
            cloner.changeSymbol(debugOpSymbol, s);
        }
    }

    // For each bound builder, this action debugger could also be called for
    //  the control flow path that comes from that builder. We set up an initial switch
    //  to direct control flow to the appropriate incoming control flow path in the
    //  operation. If control is flowing into this operation for the first time,
    //  control will flow to the cloned operation (see below). But if control is
    //  flowing from one of the bound/targeted builders, we set up a switch statement
    //  to direct control to the appropriate path in the cloned operation. We create
    //  a new builder object and append it to each corresponding builder handler after
    //  the Return. Control will flow from this new builder object to where ever the
    //  operation dictates. One could consider the Return in the builder handler to be
    //  "saving" the state of this operation, and the Switch is used to "restart" this
    //  operation at the point it left.
    Builder **caseBuilders = NULL;
    Case **cases = NULL;
    int32_t numCases = numBoundBuilders;
    if (numBoundBuilders > 0) {
        caseBuilders = new Builder *[numCases];
        cases = new Case *[numCases];
        int32_t caseIdx = 0;

        for (int32_t bIdx=0;bIdx < _op->numBuilders();bIdx++) {
            Builder *builder = _op->builder(bIdx);
            if (builder->isBound() && _op == builder->boundToOperation()) {
                Builder *restartTarget = _base->OrphanBuilder(LOC, entry);
                cloner.builder(bIdx)->AppendBuilder(restartTarget);
   
                caseBuilders[caseIdx] = _base->OrphanBuilder(LOC, entry);
                _base->Goto(LOC, caseBuilders[caseIdx], restartTarget);
                cases[caseIdx] = Case::create(builder->id(), caseBuilders[caseIdx], false);
                caseIdx++;
            }
        }
        assert(caseIdx == numCases);
    }

    Builder *firstEntry = _base->OrphanBuilder(LOC, entry);
    if (numCases > 0) {
        // safer would be to add parent builder ID to the set of cases and have a
        // default case that throws some kind of debug error; for now default is firstEntry
        _base->Switch(LOC, entry, _base->Load(LOC, entry, _fromBuilderIDSym), firstEntry, numCases, cases);
    }
    else
        _base->Goto(LOC, entry, firstEntry);

    if (_op->action() == _base->aReturn) {
        // for Return, just copy any operands to the frame's return values
        for (int32_t oIdx=0;oIdx < _op->numOperands();oIdx++) {
            storeReturnValue(LOC, fc, firstEntry, oIdx, cloner.operand(oIdx));
        }
    } else {
        Operation *cloneOp = cloner.clone(firstEntry);

        // store any results produced by the cloned operation to the appropriate DebugValues
        // (the result values produced by the original operation)
        assert(cloneOp->numResults() == _op->numResults());
        auto cloneIt = cloneOp->ResultsBegin();
        for (auto rIt = _op->ResultsBegin(); rIt != _op->ResultsEnd(); rIt++, cloneIt++) {
            Value *result = *rIt;
            Value *cloneResult = *cloneIt;
            storeValue(LOC, fc, firstEntry, result, cloneResult);
        }

        handleLocalsOutgoing(entry);
    }

    if (orphanTargets) {
        // have to put orphan builders some place
        Builder *merge = _base->OrphanBuilder(LOC, entry);
        _base->Goto(LOC, entry, merge);
        _base->AppendBuilder(orphanTargets);
        _base->AppendBuilder(merge);
    }

    _base->Return(LOC, entry, _base->ConstInt8(LOC, entry, 0));

    return true;
}

} // namespace Debug
} // namespace JB2
} // namespace OMR
