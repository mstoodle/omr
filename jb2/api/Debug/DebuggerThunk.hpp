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

#include "Debug/DebuggerFunction.hpp"

namespace OMR {
namespace JitBuilder {

namespace Base { class FunctionSymbol; }
namespace Base { class LocalSymbol; }
namespace Base { class ParameterSymbol; }

namespace Debug {

class DebuggerThunk : public DebuggerFunction {
public:
    DebuggerThunk(Debugger *dbgr, Base::Function *debugFunc)
        : DebuggerFunction(LOC, dbgr, debugFunc)
        , _debugger(dbgr) {

        DefineName(std::string("jbdb_") + debugFunc->name());
        DefineFile(debugFunc->fileName());
        DefineLine(debugFunc->lineNumber());
    }

    virtual bool initContext(Base::FunctionCompilation *comp, Base::FunctionContext *fc) {
        Base::FunctionContext *debugfc = comp->funcContext();
        for (auto pIt = debugfc->ParametersBegin(); pIt != debugfc->ParametersEnd(); pIt++) {
            const Base::ParameterSymbol *param = *pIt;
            fc->DefineParameter(param->name(), param->type());
        }
        fc->DefineReturnType(debugfc->returnType());

        _debugFuncSym = fc->DefineFunction(LOC, "debugFunction()", __FILE__, DEBUGFUNCTION_LINENUMBER, reinterpret_cast<void *>(&Debugger::debugFunction),
                                           _base->NoType, 4, _base->Address, _base->Address, _pDebugValue, _pDebugValue);
    }

    virtual bool buildIL(Base::FunctionCompilation *comp, Base::FunctionContext *fc) {
        Base::FunctionContext *debugfc = comp->funcContext();
        Builder *entry = fc->builderEntryPoint();

        Base::LocalSymbol *returnValuesSym = fc->DefineLocal("returnValues", dbgDict()->_pDebugValue);
        int32_t numReturnValues = fc->numReturnTypes();
        Value *rvPtr = NULL;
        if (numReturnValues > 0)
            rvPtr = _base->CreateLocalArray(LOC, entry, new Literal(LOC, comp, _base->Int32, (LiteralBytes *)&numReturnValues), dbgDict()->_DebugValue);
        else
            rvPtr = _base->ConstAddress(LOC, entry, 0);
        _base->Store(LOC, entry, returnValuesSym, _base->CoercePointer(LOC, entry, _pDebugValue, rvPtr));

        Base::LocalSymbol *localsSym = fc->DefineLocal("locals", dbgDict()->_pDebugValue);
        int32_t numLocals = comp->numLocals();
        Value *lPtr = NULL;
        if (numLocals > 0)
            lPtr = _base->CreateLocalArray(LOC, entry, numLocals, dbgDict()->_DebugValue);
        else
            lPtr = _base->ConstAddress(LOC, entry, 0);
        _base->Store(LOC, entry, localsSym, _base->CoercePointer(LOC, entry, _pDebugValue, lPtr));

        for (auto pIt = debugfc->ParametersBegin(); pIt != debugfc->ParametersEnd(); pIt++) {
            Base::ParameterSymbol *parm = *pIt;
            storeValue(LOC, entry, parm, _base->Load(LOC, entry, parm));
        }
        _base->Call(LOC, entry, _debugFuncSym, 4,
                    _base->ConstAddress(LOC, entry, _debugger),
                    _base->ConstAddress(LOC, entry, _func),
                    _base->Load(LOC, entry, returnValuesSym),
                    _base->Load(LOC, entry, localsSym));
        if (numReturnValues > 0) {
            assert(numReturnValues == 1); // only supporting one return value ffor now
            _base->Return(LOC, entry, loadFromDebugValue(LOC, entry, _base->Load(LOC, entry, returnValuesSym), _func->getReturnType()));
        } else {
            _base->Return(LOC, entry);
        }
  
        return true;
    }

protected:
    Debugger *_debugger;
    Base::FunctionSymbol *_debugFuncSym;
};

} // namespace Debug
} // namespace JitBuilder
} // namespace OMR

#endif // defined(DEBUGGERTHUNK_INCL)
