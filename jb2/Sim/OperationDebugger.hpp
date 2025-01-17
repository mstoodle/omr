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

#ifndef OPERATIONDEBUGGER_INCL
#define OPERATIONDEBUGGER_INCL

#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Debug/DebuggerFunction.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

class Debugger;
class DebuggerFrame;
class DebugValue;

class OperationDebugger : public DebuggerFunction {
public:
    OperationDebugger(LOCATION, Debugger *dbgr, Base::FunctionCompilation *comp, Operation *op);

    virtual bool debug(DebuggerFrame *frame, Operation *op);

protected:
    void setDebuggerBuilderTarget(Builder *b, Builder *targetBuilder);
    String valueName(Value *v);
    void handleLocalsAndValuesIncoming(Builder *b);
    void handleLocalsOutgoing(Builder *b);

    void copyResult(DebugValue *dest, DebugValue *src);

    virtual bool buildContext(LOCATION, Base::FunctionCompilation *comp, Base::FunctionContext *fc);
    virtual bool buildIL(LOCATION, Base::FunctionCompilation *comp, Base::FunctionContext *fc);

    Operation  *_op;
    String _dbgrName;
    String _localsName;
    String _valuesName;
    String _frameName;
    String _fromBuilderID;
    Base::LocalSymbol *_dbgrSym;
    Base::LocalSymbol *_localsSym;
    Base::LocalSymbol *_valuesSym;
    Base::LocalSymbol *_fromBuilderIDSym;
    Base::ParameterSymbol *_frameSym;
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(OPERATIONDEBUGGER_INCL)
