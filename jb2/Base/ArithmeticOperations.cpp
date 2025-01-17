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

#include <stdint.h>
#include "Base/ArithmeticOperations.hpp"
#include "Base/BaseExtension.hpp"
#include "Base/BaseSymbol.hpp"
#include "Base/BaseTypes.hpp"


namespace OMR {
namespace JB2 {
namespace Base {

//
// Add
//
INIT_JBALLOC_REUSECAT(Op_Add, Operation)

Op_Add::Op_Add(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aAdd, Value *result, Value *left, Value *right)
    : OperationR1V2(MEM_PASSLOC(a), aAdd, ext, parent, result, left, right) {

}

Op_Add::~Op_Add() {

}

Operation *
Op_Add::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Add(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}


//
// And
//
INIT_JBALLOC_REUSECAT(Op_And, Operation)

Op_And::Op_And(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aAnd, Value *result, Value *left, Value *right)
    : OperationR1V2(MEM_PASSLOC(a), aAnd, ext, parent, result, left, right) {

}

Op_And::~Op_And() {

}

Operation *
Op_And::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_And(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}


//
// ConvertTo
//
INIT_JBALLOC_REUSECAT(Op_ConvertTo, Operation)

Op_ConvertTo::Op_ConvertTo(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aConvertTo, Value *result, const Type *type, Value *value)
    : OperationR1T1V1(MEM_PASSLOC(a), aConvertTo, ext, parent, result, type, value) {

}

Op_ConvertTo::~Op_ConvertTo() {

}

Operation *
Op_ConvertTo::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_ConvertTo(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->type(), cloner->operand());
}


//
// Div
//
INIT_JBALLOC_REUSECAT(Op_Div, Operation)

Op_Div::Op_Div(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aDiv, Value *result, Value *left, Value *right)
    : OperationR1V2(MEM_PASSLOC(a), aDiv, ext, parent, result, left, right) {

}

Op_Div::~Op_Div() {

}

Operation *
Op_Div::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Div(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}


//
// EqualTo
//
INIT_JBALLOC_REUSECAT(Op_EqualTo, Operation)

Op_EqualTo::Op_EqualTo(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aEqualTo, Value *result, Value *left, Value *right)
    : OperationR1V2(MEM_PASSLOC(a), aEqualTo, ext, parent, result, left, right) {

}

Op_EqualTo::~Op_EqualTo() {

}

Operation *
Op_EqualTo::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_EqualTo(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}


//
// Mul
//
INIT_JBALLOC_REUSECAT(Op_Mul, Operation)

Op_Mul::Op_Mul(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aMul, Value *result, Value *left, Value *right)
    : OperationR1V2(MEM_PASSLOC(a), aMul, ext, parent, result, left, right) {

}

Op_Mul::~Op_Mul() {

}

Operation *
Op_Mul::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Mul(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}


//
// NotEqualTo
//
INIT_JBALLOC_REUSECAT(Op_NotEqualTo, Operation)

Op_NotEqualTo::Op_NotEqualTo(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aNotEqualTo, Value *result, Value *left, Value *right)
    : OperationR1V2(MEM_PASSLOC(a), aNotEqualTo, ext, parent, result, left, right) {

}

Op_NotEqualTo::~Op_NotEqualTo() {

}

Operation *
Op_NotEqualTo::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_NotEqualTo(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}


//
// Sub
//
INIT_JBALLOC_REUSECAT(Op_Sub, Operation)

Op_Sub::Op_Sub(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aSub, Value *result, Value *left, Value *right)
    : OperationR1V2(MEM_PASSLOC(a), aSub, ext, parent, result, left, right) {

}

Op_Sub::~Op_Sub() {

}

Operation *
Op_Sub::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Sub(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}


///////////////////////////////////////////////////////////////////////////////////
#if 0
// keep these here so they're handy during the migration
void
Add::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->Add(operandMappers[0]->next(), operandMappers[1]->next()) );
   }

Operation *
Add::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->operand(0), cloner->operand(1));
   }


void
Sub::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->Sub(operandMappers[0]->next(), operandMappers[1]->next()) );
   }

Operation *
Sub::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->operand(0), cloner->operand(1));
   }


void
Mul::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->Mul(operandMappers[0]->next(), operandMappers[1]->next()) );
   }

Operation *
Mul::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->operand(0), cloner->operand(1));
   }


#endif

} // namespace Base
} // namespace JB2
} // namespace OMR
