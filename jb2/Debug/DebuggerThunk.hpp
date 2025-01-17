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

#ifndef DEBUGGERTHUNK_INCL
#define DEBUGGERTHUNK_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"


namespace OMR {
namespace JB2 {
namespace Debug {

class DebuggerThunk : public Func::Function {
public:
    DebuggerThunk(MEM_LOCATION(a), Debugger *jbdb, IR *ir)
        : Func::Function(MEMLOC(a), jbdb->compiler())
        , _jbdb(jbdb)
        , _contextToDebug(ir->context<Func::FunctionContext>()) {

        Func::Function *funcToDebug=ir->unit()->refine<Func::Function>();
        DefineName(String(a, "Debug::") + funcToDebug->name());
        DefineFile(funcToDebug->fileName());
        DefineLine(funcToDebug->lineNumber());
    }

    virtual bool buildContext(DebugCompilation *comp, Func::FunctionScope *scope, DebugContext *ctx) {
        // signature should match the original function's
        for (auto pIt = _contextToDebug->parameters(); pIt.hasItem(); pIt++) {
            const Func::ParameterSymbol *param = pIt.item();
            fc->DefineParameter(param->name(), param->type());
        }
        ctx->DefineReturnType(_contextToDebug->returnType());

        // entry point to the Debugger
        _debugFuncSym = fc->DefineFunction(LOC, "debugFunction()", __FILE__, DEBUGFUNCTION_LINENUMBER, reinterpret_cast<void *>(&Debugger::debugFunction),
                                           _bx->NoType, 4, _bx->Address, _bx->Address, comp->_pDebugValue, comp->_pDebugValue);
    }

    virtual bool buildIL(DebugCompilation *comp, Func::FunctionScope *scope, DebugContext *ctx) {
        Builder *entry = ctx->entryPoint<BuilderEntry>()->builder();

        // allocate debug return values array
        Func::LocalSymbol *returnValuesSym = ctx->DefineLocal("returnValues", comp->_pDebugValue);
        int32_t numReturnValues = ctx->numReturnTypes();
        Value *rvPtr = NULL;
        if (numReturnValues > 0)
            rvPtr = _bx->CreateLocalArray(LOC, entry, new Literal(LOC, comp, _bx->Int32, (LiteralBytes *)&numReturnValues), comp->_DebugValue);
        else
            rvPtr = _bx->ConstAddress(LOC, entry, 0);
        _fx->Store(LOC, entry, returnValuesSym, _bx->CoercePointer(LOC, entry, comp->_pDebugValue, rvPtr));

        // allocate the debug locals array
        Func::LocalSymbol *localsSym = ctx->DefineLocal("locals", comp->_pDebugValue);
        int32_t numLocals = ctx->numLocals();
        Value *lPtr = NULL;
        if (numLocals > 0)
            lPtr = _bx->CreateLocalArray(LOC, entry, numLocals, comp->_DebugValue);
        else
            lPtr = _bx->ConstAddress(LOC, entry, 0);
        _fx->Store(LOC, entry, localsSym, _bx->CoercePointer(LOC, entry, comp->_pDebugValue, lPtr));

        // store arguments we were passed into the debug locals
        for (auto pIt = ctx->parameters(); pIt.hasItem(); pIt++) {
            Func::ParameterSymbol *parm = pIt.item();
            ctx->storeValue(LOC, entry, parm, _fx->Load(LOC, entry, parm));
        }
        
        // enter the debugger
        _bx->Call(LOC, entry, _debugFuncSym, 4,
                  _bx->ConstAddress(LOC, entry, _jbdb),
                  _bx->ConstAddress(LOC, entry, _func),
                  _fx->Load(LOC, entry, returnValuesSym),
                  _fx->Load(LOC, entry, localsSym));

        // read the return value(s), if there are any, from the debug locals and return them to the caller
        // only handles no or single return value for now
        if (numReturnValues > 0) {
            assert(numReturnValues == 1); // only supporting one return value ffor now
            _fx->Return(LOC, entry, ctx->loadFromDebugValue(LOC, entry, _fx->Load(LOC, entry, returnValuesSym), ctx->getReturnType()));
        } else {
            _fx->Return(LOC, entry);
        }
  
        return true;
    }

protected:
    Debugger *_jbdb;
    Func::FunctionContext *_contextToDebug;
    Base::FunctionSymbol *_debugFuncSym;
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(DEBUGGERTHUNK_INCL)
