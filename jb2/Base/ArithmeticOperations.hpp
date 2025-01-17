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

#ifndef ARITHMETICOPERATIONS_INCL
#define ARITHMETICOPERATIONS_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

DECL_OPERATION_CLASS(Op_Add, OperationR1V2, BaseExtension,
    Op_Add(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aAdd, Value *result, Value *left, Value *right);
)

DECL_OPERATION_CLASS(Op_And, OperationR1V2, BaseExtension,
    Op_And(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aAnd, Value *result, Value *left, Value *right);
)

DECL_OPERATION_CLASS(Op_ConvertTo, OperationR1T1V1, BaseExtension,
    Op_ConvertTo(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aConvertTo, Value *result, const Type *type, Value *value);
)

DECL_OPERATION_CLASS(Op_Div, OperationR1V2, BaseExtension,
    Op_Div(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aDiv, Value *result, Value *left, Value *right);
)

DECL_OPERATION_CLASS(Op_EqualTo, OperationR1V2, BaseExtension,
    Op_EqualTo(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aEqualTo, Value *result, Value *left, Value *right);
)

DECL_OPERATION_CLASS(Op_Mul, OperationR1V2, BaseExtension,
    Op_Mul(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aMul, Value *result, Value *left, Value *right);
)

DECL_OPERATION_CLASS(Op_NotEqualTo, OperationR1V2, BaseExtension,
    Op_NotEqualTo(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aNotEqualTo, Value *result, Value *left, Value *right);
)

DECL_OPERATION_CLASS(Op_Sub, OperationR1V2, BaseExtension,
    Op_Sub(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aSub, Value *result, Value *left, Value *right);
)

} // namespace Base
} // namespace JB2
} // namespace OMR

#endif // !defined(ARITHMETICOPERATIONS_INCL)
