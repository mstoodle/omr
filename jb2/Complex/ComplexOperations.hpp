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

#ifndef COMPLEXOPERATIONS_INCL
#define COMPLEXOPERATIONS_INCL

#include "Literal.hpp"
#include "Operation.hpp"

namespace OMR {
namespace JB2 {
namespace Complex {

class Op_Real : public OperationR1V1 {
    friend class ComplexExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_Real(LOCATION, Extension *ext, Builder * parent, ActionID aReal, Value *result, Value *value);
    };

class Op_Imag : public OperationR1V1 {
    friend class ComplexExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_Imag(LOCATION, Extension *ext, Builder * parent, ActionID aImag, Value *result, Value *value);
    };

class Op_Conjugate : public OperationR1V1 {
    friend class ComplexExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_Conjugate(LOCATION, Extension *ext, Builder * parent, ActionID aConjugate, Value *result, Value *value);
    };

class Op_Magnitude : public OperationR1V1 {
    friend class ComplexExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void jbgen(JB1MethodBuilder *j1mb) const;

protected:
    Op_Magnitude(LOCATION, Extension *ext, Builder * parent, ActionID aMagnitude, Value *result, Value *value);
    };

} // namespace Complex
} // namespace JB2
} // namespace OMR

#endif // !defined(COMPLEXOPERATIONS_INCL)
