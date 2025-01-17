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

#ifndef FUNCTIONOPERATIONS_INCL
#define FUNCTIONOPERATIONS_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class FunctionExtension;
class FunctionSymbol;

DECL_OPERATION_CLASS(Op_Load, OperationR1S1, FunctionExtension,
    Op_Load(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aLoad, Value *result, Symbol *s);
)

DECL_OPERATION_CLASS(Op_Store, OperationR0S1V1, FunctionExtension,
    Op_Store(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aStore, Symbol *s, Value *value);
)

DECL_OPERATION_CLASS(Op_Call, OperationR1S1VN, FunctionExtension,
    Op_Call(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCall, Value *result, FunctionSymbol *target, std::va_list & args);
    Op_Call(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCall, OperationCloner *cloner)
        : OperationR1S1VN(MEM_PASSLOC(a), aCall, ext, parent, cloner) {
    }
public:
    virtual void log(TextLogger &lgr) const;
)

DECL_OPERATION_CLASS(Op_CallVoid, OperationR0S1VN, FunctionExtension,
    Op_CallVoid(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCallVoid, FunctionSymbol *target, std::va_list & args);
    Op_CallVoid(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCallVoid, OperationCloner *cloner)
        : OperationR0S1VN(MEM_PASSLOC(a), aCallVoid, ext, parent, cloner) {
    }
public:
    virtual void log(TextLogger &lgr) const;
)

DECL_OPERATION_CLASS(Op_ReturnVoid, Operation, FunctionExtension,
    Op_ReturnVoid(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aReturnVoid);
)

// eventually generalize to handle multiple return values but not needed yet
DECL_OPERATION_CLASS(Op_Return, OperationR0V1, FunctionExtension,
    Op_Return(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aReturn, Value * v);
)

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // !defined(FUNCTIONOPERATIONS_INCL)
