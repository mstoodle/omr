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

#include <assert.h>
#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"
#include "Debug/DebugCompilation.hpp"
#include "Debug/DebugContext.hpp"
#include "Debug/DebugExtension.hpp"
#include "Debug/DebuggerFunction.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

DebugFunction::DebugFunction(MEM_LOCATION(a), Compiler *compiler, Compilation *compToDebug, Operation *opToDebug)
    : Func::Function(MEM_PASSLOC(a), compiler)
    , _cx(compiler->coreExt())
    , _fx(compiler->lookupExtension<Func::FunctionExtension>())
    , _bx(compiler->lookupExtension<Base::BaseExtension>())
    , _dbgrName("debugger")
    , _localsName("locals")
    , _valuesName("values")
    , _frameName("frame")
    , _fromBuilderIDName("fromBuilderID")
    , _compToDebug(compToDebug)
    , _opToDebug(opToDebug) {

    DefineName(compToDebug->unit()->name() + String(a, ".op") + String::to_string(a, opToDebug->id()));
    DefineFile("OpDbgr");
    DefineLine(opToDebug->name());
}

bool
DebugFunction::buildContext(LOCATION, Func::FunctionCompilation *fcomp, Func::FunctionScope *scope, Func::FunctionContext *fctx) {
    DebugCompilation *comp = fcomp->refine<DebugCompilation>();
    DebugContext *ctx = fctx->refine<DebugContext>();

    ctx->DefineLocal(_dbgrName, comp->_pDebugValue);
    ctx->DefineLocal(_localsName, comp->_pDebugValue);
    ctx->DefineLocal(_valuesName, comp->_pDebugValue);

    ctx->DefineParameter(_frameName, comp->_pDebugFrame);
    ctx->DefineParameter(_fromBuilderIDName, _bx->Int32);

    ctx->DefineReturnType(_bx->Int8);

    return true;
}

bool
DebugFunction::buildIL(LOCATION, Func::FunctionCompilation *fcomp, Func::FunctionScope *scope, Func::FunctionContext *fctx) {
    DebugCompilation *comp = fcomp->refine<DebugCompilation>();
    DebugContext *ctx = fctx->refine<DebugContext>();

    Func::LocalSymbol *dbgrSym = ctx->LookupLocal(_dbgrName);
    Func::LocalSymbol *localsSym = ctx->LookupLocal(_localsName);
    Func::LocalSymbol *valuesSym = ctx->LookupLocal(_valuesName);
    Func::LocalSymbol *frameSym = ctx->LookupLocal(_frameName);
    Func::LocalSymbol *fromBuilderIDSym = ctx->LookupLocal(_fromBuilderIDName);

    Builder *entry = scope->entryPoint<BuilderEntry>()->builder();

    Value *frame = _fx->Load(LOC, entry, frameSym);
    _fx->Store(LOC, entry, dbgrSym, _bx->LoadFieldAt(LOC, entry, comp->_DebugFrame_debugger, frame));
    _fx->Store(LOC, entry, localsSym, _bx->LoadFieldAt(LOC, entry, comp->_DebugFrame_locals, frame));
    _fx->Store(LOC, entry, valuesSym, _bx->LoadFieldAt(LOC, entry, comp->_DebugFrame_values, frame));

    //handleLocalsAndValuesIncoming(entry); handled by DebugContext?

    OperationCloner *cloner = new (comp->mem()) OperationCloner(comp->mem(), _opToDebug);

    #if 0 // think DebugContext will take care of this?
    // convert operands v of this operation to load their values from the DebugValues in the frame
    // operands *must* be loaded here so they reach all paths
    if (_op->numOperands() > 0) {
        int32_t opNum=0;
        for (auto opIt = _op->OperandsBegin(); opIt != _op->OperandsEnd(); opIt++) {
            Value *origOperand = *opIt;
            cloner->changeOperand(loadValue(LOC, fc, entry, origOperand), opNum++);
           //cloner.changeOperand(firstEntry->Load(valueName(origOperand)), opNum++);
        }
    }
    #endif

    // generate switch based on incoming builder id to simulate continuation
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
    if (_opToDebug->numBuilders() > 0) {
        orphanTargets = _cx->OrphanBuilder(LOC, _opToDebug->parent());
        for (int32_t bIdx = 0; bIdx < _opToDebug->numBuilders(); bIdx++) {
            Builder *op_b = _opToDebug->builder(bIdx);
            Builder *b = _cx->OrphanBuilder(LOC, op_b->parent());
            cloner->changeBuilder(b, bIdx);
            // handleLocalsOutgoing(b); hope DebugContext will handle this part
            _bx->StoreFieldAt(LOC, b, comp->_DebugFrame_builderToDebug, _fx->Load(LOC, b, frameSym), _bx->ConstAddress(LOC, b, (void *)op_b));
            _fx->Return(LOC, b, _bx->ConstInt8(LOC, b, 1));
         

            if (op_b->isBound() && _opToDebug == op_b->boundToOperation())
                numBoundBuilders++;
            else
                _cx->AppendBuilder(LOC, orphanTargets, b);
        }
    }

    #if 0 // handled by DebugContext?
    // need to change Symbols from those in the original FunctionBuiilder to those in this OperationDebugger
    if (_op->numSymbols() > 0) {
        for (auto s=0;s < _op->numSymbols();s++) {
            Symbol *origSymbol = _op->symbol(s);
            Symbol *debugOpSymbol = fc->getSymbol(origSymbol->name());
            cloner->changeSymbol(debugOpSymbol, s);
        }
    }
    #endif

    // For each bound builder, this action debugger could also be called for
    //  the control flow path that comes back from that builder. We set up an initial switch
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
    Base::SwitchBuilder bldr(comp->mem());
    Builder **caseBuilders = NULL;
    Case **cases = NULL;
    int32_t numCases = numBoundBuilders;
    if (numBoundBuilders > 0) {
        caseBuilders = new Builder *[numCases];
        cases = new Case *[numCases];
        int32_t caseIdx = 0;

        for (int32_t bIdx=0;bIdx < _opToDebug->numBuilders();bIdx++) {
            Builder *builder = _opToDebug->builder(bIdx);
            if (builder->isBound() && _opToDebug == builder->boundToOperation()) {
                Builder *restartTarget = _cx->OrphanBuilder(LOC, entry);
                _cx->AppendBuilder(LOC, cloner->builder(bIdx), restartTarget);
   
                caseBuilders[caseIdx] = _cx->OrphanBuilder(LOC, entry);
                _bx->Goto(LOC, caseBuilders[caseIdx], restartTarget);
                bldr.addCase(_bx->Int32->literal(LOC, comp, (int32_t)builder->id()), caseBuilders[caseIdx], false);
                caseIdx++;
            }
        }
        assert(caseIdx == numCases);
    }

    Builder *firstEntry = _cx->OrphanBuilder(LOC, entry);
    if (numCases > 0) {
        // safer would be to add parent builder ID to the set of cases and have a
        // default case that throws some kind of debug error; for now default is firstEntry
        bldr.setSelector(_fx->Load(LOC, entry, fromBuilderIDSym))
            ->setDefaultBuilder(firstEntry);
        _bx->Switch(LOC, entry, &bldr);
    }
    else
        _bx->Goto(LOC, entry, firstEntry);

    #if 0 // can DebugContext handle all this?
    if (_opToDebug->action() == _fx->aReturn) {
        // for Return, just copy any operands to the frame's return values
        for (int32_t oIdx=0;oIdx < _op->numOperands();oIdx++) {
            storeReturnValue(LOC, fc, firstEntry, oIdx, cloner->operand(oIdx));
        }
    } else {
        Operation *cloneOp = cloner->clone(firstEntry);

        // store any results produced by the cloned operation to the appropriate DebugValues
        // (the result values produced by the original operation)
        assert(cloneOp->numResults() == _opToDebug->numResults());
        auto cloneIt = cloneOp->ResultsBegin();
        for (auto rIt = _op->ResultsBegin(); rIt != _op->ResultsEnd(); rIt++, cloneIt++) {
            Value *result = *rIt;
            Value *cloneResult = *cloneIt;
            storeValue(LOC, fc, firstEntry, result, cloneResult);
        }

        handleLocalsOutgoing(entry);
    }
    #endif

    Operation *cloneOp = cloner->clone(firstEntry);
    comp->mem()->deallocate(cloner);

    if (orphanTargets) {
        // have to put orphan builders some place
        Builder *merge = _cx->OrphanBuilder(LOC, entry);
        _bx->Goto(LOC, entry, merge);
        //_cx->AppendBuilder(LOC, orphanTargets);
        //_cx->AppendBuilder(LOC, merge);
    }

    _fx->Return(LOC, entry, _bx->ConstInt8(LOC, entry, 0));

    return true;
}

} // namespace Debug
} // namespace JB2
} // namespace OMR
