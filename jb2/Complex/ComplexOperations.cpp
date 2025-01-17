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
#include "JBCore.hpp"
#include "ComplexExtension.hpp"
#include "ComplexOperations.hpp"

using namespace OMR::JB2;

namespace OMR {
namespace JB2 {
namespace Complex {

//
// Real
//
INIT_JBALLOC(Op_Real)

Op_Real::Op_Real(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aReal, Value *result, Value *value)
    : OperationR1V1(MEM_PASSLOC(a), aReal, ext, parent, result, value) {
}

Op_Real::~Op_Real() {

}

Operation *
Op_Real::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new (b->allocator()) Op_Real(MEM_PASSLOC(b->allocator()), this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

bool
Op_Real::expand(Builder *b, OperationReplacer *r) const {
    // Real just selects the "real" part of its operand, so just map the result to that value
    Value *realPart = r->operandMapper(0)->next();
    //r->setResult(realPart);
    Allocator *mem = r->allocator();
    r->setResultMapper(new (mem) ValueMapper(mem, realPart), 0);
    return true;
}

//
// Imag
//
INIT_JBALLOC(Op_Imag)

Op_Imag::Op_Imag(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aImag, Value *result, Value *value)
    : OperationR1V1(MEM_PASSLOC(a), aImag, ext, parent, result, value) {
}

Op_Imag::~Op_Imag() {

}

Operation *
Op_Imag::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new (b->allocator()) Op_Imag(MEM_PASSLOC(b->allocator()), this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

bool
Op_Imag::expand(Builder *b, OperationReplacer *r) const {
    // Imag just selects the "imaginary" part of its operand, so just map the result to that value
    Value *unusedRealPart = r->operandMapper(0)->next();
    Value *imagPart = r->operandMapper(0)->next();
    //r->setResult(imagPart);
    Allocator *mem = r->allocator();
    r->setResultMapper(new (mem) ValueMapper(mem, imagPart), 0);
    return true;
}


//
// Conjugate
//
INIT_JBALLOC(Op_Conjugate)

Op_Conjugate::Op_Conjugate(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aConjugate, Value *result, Value *value)
    : OperationR1V1(MEM_PASSLOC(a), aConjugate, ext, parent, result, value) {
}

Op_Conjugate::~Op_Conjugate() {

}

Operation *
Op_Conjugate::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new (b->allocator()) Op_Conjugate(MEM_PASSLOC(b->allocator()), this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

bool
Op_Conjugate::expand(Builder *b, OperationReplacer *r) const {
    Base::BaseExtension *bx = b->compiler()->lookupExtension<Base::BaseExtension>();
    Allocator *mem = r->allocator();

    // Conjugate doesn't change the "real" part so just map it as-is
    Value *realValue = r->operandMapper(0)->next();

    // "imag" part needs to be negated
    Value *imagValue = r->operandMapper(0)->next();
    Value *conjImagValue = bx->Sub(LOC, b, bx->Const(LOC, b, imagValue->type()->zero(LOC)), imagValue);

    ValueMapper *resultMapper = new (mem) ValueMapper(mem);
    resultMapper->add(realValue);
    resultMapper->add(conjImagValue);
    r->setResultMapper(resultMapper);

    return true;
}


//
// Magnitude
//
INIT_JBALLOC(Op_Magnitude)

Op_Magnitude::Op_Magnitude(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aMagnitude, Value *result, Value *value)
    : OperationR1V1(MEM_PASSLOC(a), aMagnitude, ext, parent, result, value) {
}

Op_Magnitude::~Op_Magnitude() {

}

Operation *
Op_Magnitude::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new (b->allocator()) Op_Magnitude(MEM_PASSLOC(b->allocator()), this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

bool
Op_Magnitude::expand(Builder *b, OperationReplacer *r) const {
    // don't have any sqrt so this will not work yet
    return false;
}


} // namespace Complex
} // namespace JB2
} // namespace OMR
