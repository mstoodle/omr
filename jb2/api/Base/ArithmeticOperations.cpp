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
#include "ArithmeticOperations.hpp"
#include "BaseExtension.hpp"
#include "BaseSymbols.hpp"
#include "BaseTypes.hpp"
#include "Builder.hpp"
#include "Function.hpp"
#include "JB1MethodBuilder.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "TextWriter.hpp"
#include "Value.hpp"


namespace OMR {
namespace JitBuilder {
namespace Base {

//
// Add
//
Op_Add::Op_Add(LOCATION, Extension *ext, Builder * parent, ActionID aAdd, Value *result, Value *left, Value *right)
    : OperationR1V2(PASSLOC, aAdd, ext, parent, result, left, right) {

}

Operation *
Op_Add::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Add(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}

void
Op_Add::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Add(location(), this->parent(), this->_result, this->_left, this->_right);
}


//
// ConvertTo
//
Op_ConvertTo::Op_ConvertTo(LOCATION, Extension *ext, Builder * parent, ActionID aConvertTo, Value *result, const Type *type, Value *value)
    : OperationR1V1T1(PASSLOC, aConvertTo, ext, parent, result, type, value) {

}

Operation *
Op_ConvertTo::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_ConvertTo(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->type(), cloner->operand());
}

void
Op_ConvertTo::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->ConvertTo(location(), this->parent(), this->_result, this->_type, this->_value);
}


//
// Mul
//
Op_Mul::Op_Mul(LOCATION, Extension *ext, Builder * parent, ActionID aMul, Value *result, Value *left, Value *right)
    : OperationR1V2(PASSLOC, aMul, ext, parent, result, left, right) {

}

Operation *
Op_Mul::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Mul(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}

void
Op_Mul::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Mul(location(), this->parent(), this->_result, this->_left, this->_right);
}


//
// Sub
//
Op_Sub::Op_Sub(LOCATION, Extension *ext, Builder * parent, ActionID aSub, Value *result, Value *left, Value *right)
    : OperationR1V2(PASSLOC, aSub, ext, parent, result, left, right) {

}

Operation *
Op_Sub::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Sub(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}

void
Op_Sub::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Sub(location(), this->parent(), this->_result, this->_left, this->_right);
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
} // namespace JitBuilder
} // namespace OMR
