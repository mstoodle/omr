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

#include "Literal.hpp"
#include "Operation.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

class Op_Add : public OperationR1V2 {
    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_Add(LOCATION, Extension *ext, Builder * parent, ActionID aAdd, Value *result, Value *left, Value *right);
    };

class Op_ConvertTo : public OperationR1V1T1 {
    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_ConvertTo(LOCATION, Extension *ext, Builder * parent, ActionID aConvertTo, Value *result, const Type *type, Value *value);
    };

class Op_Mul : public OperationR1V2 {
    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_Mul(LOCATION, Extension *ext, Builder * parent, ActionID aMul, Value *result, Value *left, Value *right);
    };

class Op_Sub : public OperationR1V2 {
    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_Sub(LOCATION, Extension *ext, Builder * parent, ActionID aSub, Value *result, Value *left, Value *right);
    };

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(ARITHMETICOPERATIONS_INCL)

