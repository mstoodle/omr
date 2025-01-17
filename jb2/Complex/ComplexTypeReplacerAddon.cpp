/*******************************************************************************
 * Copyright (c) 2026 IBM Corp. and others
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

#include "JBCore.hpp"
#include "ComplexExtension.hpp"
#include "ComplexIRAddon.hpp"
#include "ComplexTypeReplacerAddon.hpp"

namespace OMR {
namespace JB2 {
namespace Complex {

INIT_JBALLOC_REUSECAT(ComplexTypeReplacerAddon, TypeReplacer)
SUBCLASS_KINDSERVICE_IMPL(ComplexTypeReplacerAddon, "ComplexTypeReplacerAddon", TypeReplacerAddon, Extensible)

ComplexTypeReplacerAddon::ComplexTypeReplacerAddon(Allocator *a, ComplexExtension *cx, TypeReplacer *repl)
    : TypeReplacerAddon(a, cx, repl, KIND(Extensible)) {
}

ComplexTypeReplacerAddon::~ComplexTypeReplacerAddon() {

}

void
ComplexTypeReplacerAddon::collectTypes(IR *ir) const {
    TypeReplacer *repl = root()->refine<TypeReplacer>();
    ComplexIRAddon *ciao = ir->addon<ComplexIRAddon>();
    repl->explode(ciao->ComplexFloat32)
        ->explode(ciao->ComplexFloat64);
}

bool
ComplexTypeReplacerAddon::hasExpander(Operation *op) const {
    Base::BaseExtension *bx = compiler()->lookupExtension<Base::BaseExtension>();
    IR *ir = op->ir();
    ComplexIRAddon *ciao = ir->addon<ComplexIRAddon>();
    TypeReplacer *repl = root()->refine<TypeReplacer>();
    
    if (op->action() == bx->aMul) {
        Value *left = op->operand(0);
        Value *right = op->operand(1);
        const Type *leftType = left->type();
        const Type *rightType = right->type();

        if (leftType != ciao->ComplexFloat32 && leftType != ciao->ComplexFloat64)
            return false;
        if (rightType != ciao->ComplexFloat32 && rightType != ciao->ComplexFloat64)
            return false;
        // left and right should be same type now
        assert(leftType == rightType);

        // but it's possible this TypeReplacer pass isn't exploding Complex types
        return (repl->isTypeToExplode(leftType));

    } else if (op->action() == bx->aLoadAt) {
        Value *result = op->result(0);
        const Type *resultType = result->type();
        return (resultType == ciao->ComplexFloat32 || resultType == ciao->ComplexFloat64) && repl->isTypeToExplode(resultType);
        
    } else if (op->action() == bx->aStoreAt) {
        Value *value = op->operand(1);
        const Type *valueType = value->type();
        return (valueType == ciao->ComplexFloat32 || valueType == ciao->ComplexFloat64) && repl->isTypeToExplode(valueType);

    }

    return false;
}

bool
ComplexTypeReplacerAddon::expand(Builder *b, Operation *op, OperationReplacer *r) const {
    Base::BaseExtension *bx = compiler()->lookupExtension<Base::BaseExtension>();
    IR *ir = op->ir();
    ComplexIRAddon *ciao = ir->addon<ComplexIRAddon>();

    if (op->action() == bx->aMul) {
        ValueMapper *leftMapper = r->operandMapper(0);
        ValueMapper *rightMapper = r->operandMapper(1);

        // cross multiply the elements and add corresponding real, imag parts
        Value * l_real = leftMapper->next();
        Value * l_imag = leftMapper->next();
        Value * r_real = rightMapper->next();
        Value * r_imag = rightMapper->next();

        Value *real_squared = bx->Mul(LOC, b, l_real, r_real);
        Value *imag_squared = bx->Mul(LOC, b, l_imag, r_imag);
        Value *resultReal = bx->Sub(LOC, b, real_squared, imag_squared);

        Value *first_cross = bx->Mul(LOC, b, l_real, r_imag);
        Value *second_cross = bx->Mul(LOC, b, l_imag, r_real);
        Value *resultImag = bx->Add(LOC, b, first_cross, second_cross);

        ValueMapper *resultMapper = r->resultMapper();
        resultMapper->add(resultReal);
        resultMapper->add(resultImag);

        return true;

    } else if (op->action() == bx->aLoadAt) {
        ValueMapper *baseMapper = r->operandMapper(0);
        Value *basePointer = baseMapper->next();

        const Type *resultType = op->result()->type();
        const Base::StructType *layoutType = resultType->layout()->refine<Base::StructType>();
        const Base::FieldType *fieldTypeReal = layoutType->LookupField("real");
        const Base::FieldType *fieldTypeImag = layoutType->LookupField("imag");

        Value *resultReal = bx->LoadFieldAt(LOC, b, fieldTypeReal, basePointer);
        Value *resultImag = bx->LoadFieldAt(LOC, b, fieldTypeImag, basePointer);

        ValueMapper *resultMapper = r->resultMapper();
        resultMapper->add(resultReal);
        resultMapper->add(resultImag);

        return true;

    } else if (op->action() == bx->aStoreAt) {
        ValueMapper *baseMapper = r->operandMapper(0);
        Value *basePointer = baseMapper->next();

        const Type *operandType = op->operand(1)->type();
        const Base::StructType *layoutType = operandType->layout()->refine<Base::StructType>();
        const Base::FieldType *fieldTypeReal = layoutType->LookupField("real");
        const Base::FieldType *fieldTypeImag = layoutType->LookupField("imag");

        ValueMapper *valueMapper = r->operandMapper(1);
        Value *valueReal = valueMapper->next();
        Value *valueImag = valueMapper->next();

        bx->StoreFieldAt(LOC, b, fieldTypeReal, basePointer, valueReal);
        bx->StoreFieldAt(LOC, b, fieldTypeImag, basePointer, valueImag);

        return true;
    }

    return false;
}

} // namespace Complex
} // namespace JB2
} // namespace OMR
