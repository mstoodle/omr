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

#include "Builder.hpp"
#include "ComplexExtension.hpp"
#include "ComplexIRAddon.hpp"
#include "ComplexOperations.hpp"
#include "ComplexTypeReplacerAddon.hpp"
#include "ComplexTypes.hpp"
#include "Compiler.hpp"
#include "Extension.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "TypeReplacer.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {
namespace Complex {

INIT_JBALLOC_REUSECAT(ComplexExtension, Extension)
SUBCLASS_KINDSERVICE_IMPL(ComplexExtension,"ComplexExtension",Extension,Extensible);

INIT_JBALLOC_REUSECAT(ComplexExtensionChecker, Extension)
INIT_JBALLOC_REUSECAT(Complex_BaseExtensionChecker, Extension)

const SemanticVersion ComplexExtension::version(COMPLEXEXT_MAJOR,COMPLEXEXT_MINOR,COMPLEXEXT_PATCH);
const SemanticVersion ComplexExtension::requiredBaseVersion(REQUIRED_BASEEXT_MAJOR,REQUIRED_BASEEXT_MINOR,REQUIRED_BASEEXT_PATCH);
const String ComplexExtension::NAME("jb2complex");

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Allocator *mem = compiler->mem();
        return new (mem) ComplexExtension(MEM_PASSLOC(mem), compiler);
    }
}

ComplexExtension::ComplexExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(MEM_PASSLOC(a), CLASSKIND(ComplexExtension,Extensible), compiler, (extended ? extensionName : NAME))
    , _base(compiler->loadExtension<Base::BaseExtension>(PASSLOC, &requiredBaseVersion))
    , aReal(registerAction(String(a, "Real")))
    , aImag(registerAction(String(a, "Imag")))
    , aConjugate(registerAction(String(a, "Conjugate")))
    , aMagnitude(registerAction(String(a, "Magnitude")))
    , CompileFail_BadInputTypes_Real(registerReturnCode(String(a, "CompileFail_BadInputTypes_Real")))
    , CompileFail_BadInputTypes_Imag(registerReturnCode(String(a, "CompileFail_BadInputTypes_Imag")))
    , CompileFail_BadInputTypes_Conjugate(registerReturnCode(String(a, "CompileFail_BadInputTypes_Conjugate")))
    , CompileFail_BadInputTypes_Magnitude(registerReturnCode(String(a, "CompileFail_BadInputTypes_Magnitude")))
    , _checkers(NULL, a) {

    if (!extended) {
        _checkers.push_back(new (a) ComplexExtensionChecker(a, this));
    }

    registerForExtensible(CLASSKIND(IR,Extensible), this);
    registerForExtensible(CLASSKIND(TypeReplacer,Extensible), this);

    createAddon(compiler->irPrototype());
    createAddon(compiler->coreExt()->typeReplacer());

    if (_base != NULL)
        _base->registerChecker(new (a) Complex_BaseExtensionChecker(a, this, _base));
}

ComplexExtension::~ComplexExtension() {
    for (auto it=_checkers.iterator();it.hasItem();it++) {
        ComplexExtensionChecker *checker = it.item();
        delete checker;
    }
}

void
ComplexExtension::registerChecker(ComplexExtensionChecker *checker) {
    _checkers.push_front(checker);
}

void
ComplexExtension::createAddon(Extensible *e) {
    Allocator *mem = e->allocator();

    if (e->isKind<IR>()) {
        ComplexIRAddon *cira = new (mem) ComplexIRAddon(mem, this, e->refine<IR>());
        e->attach(cira);
    } else if (e->isKind<TypeReplacer>()) {
        ComplexTypeReplacerAddon *ctra = new (mem) ComplexTypeReplacerAddon(mem, this, e->refine<TypeReplacer>());
        e->attach(ctra);
    }
}

const ComplexFloat32Type *
ComplexExtension::ComplexFloat32(IR *ir) const {
    return ir->addon<ComplexIRAddon>()->ComplexFloat32;
}

const ComplexFloat64Type *
ComplexExtension::ComplexFloat64(IR *ir) const {
    return ir->addon<ComplexIRAddon>()->ComplexFloat64;
}

//
// Operations
//
bool
ComplexExtensionChecker::validateReal(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (!type->isKind<ComplexType>()) {
        failValidateReal(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateReal(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Real);
    e.setMessageLine(String("Real: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string(b->allocator())))
     .appendMessageLine(String("value must be one of ComplexFloat32, ComplexFloat64)"));
    throw e;
}

Value *
ComplexExtension::Real(LOCATION, Builder *b, Value *value) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        ComplexExtensionChecker *checker = it.item();
        if (checker->validateReal(PASSLOC, b, value))
            break;
    }

    Value *result = createValue(b, static_cast<const ComplexType *>(value->type())->elementType());
    addOperation(b, new (b->allocator()) Op_Real(MEM_PASSLOC(b->allocator()), this, b, aReal, result, value));
    return result;
}

bool
ComplexExtensionChecker::validateImag(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (!type->isKind<ComplexType>()) {
        failValidateImag(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateImag(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Imag);
    e.setMessageLine(String("Imag: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string(b->allocator())))
     .appendMessageLine(String("value must be one of ComplexFloat32, ComplexFloat64)"));
    throw e;
}

Value *
ComplexExtension::Imag(LOCATION, Builder *b, Value *value) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        ComplexExtensionChecker *checker = it.item();
        if (checker->validateImag(PASSLOC, b, value))
            break;
    }

    Value *result = createValue(b, static_cast<const ComplexType *>(value->type())->elementType());
    addOperation(b, new (b->allocator()) Op_Imag(MEM_PASSLOC(b->allocator()), this, b, aReal, result, value));
    return result;
}

bool
ComplexExtensionChecker::validateConjugate(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (!type->isKind<ComplexType>()) {
        failValidateConjugate(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateConjugate(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Conjugate);
    e.setMessageLine(String("Conjugate: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string(b->allocator())))
     .appendMessageLine(String("value must be one of ComplexFloat32, ComplexFloat64)"));
    throw e;
}

Value *
ComplexExtension::Conjugate(LOCATION, Builder *b, Value *value) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        ComplexExtensionChecker *checker = it.item();
        if (checker->validateConjugate(PASSLOC, b, value))
            break;
    }

    Value *result = createValue(b, value->type());
    addOperation(b, new (b->allocator()) Op_Conjugate(MEM_PASSLOC(b->allocator()), this, b, aReal, result, value));
    return result;
}

bool
ComplexExtensionChecker::validateMagnitude(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (!type->isKind<ComplexType>()) {
        failValidateMagnitude(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateMagnitude(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Magnitude);
    e.setMessageLine(String("Magnitude: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string(b->allocator())))
     .appendMessageLine(String("value must be one of ComplexFloat32, ComplexFloat64)"));
    throw e;
}

Value *
ComplexExtension::Magnitude(LOCATION, Builder *b, Value *value) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        ComplexExtensionChecker *checker = it.item();
        if (checker->validateMagnitude(PASSLOC, b, value))
            break;
    }

    Value *result = createValue(b, static_cast<const ComplexType *>(value->type())->elementType());
    addOperation(b, new (b->allocator()) Op_Magnitude(MEM_PASSLOC(b->allocator()), this, b, aMagnitude, result, value));
    return result;
}

//
// Pseudo operations
//
Value *
ComplexExtension::ConstComplexFloat32(LOCATION, Builder *b, std::complex<float> v) {
    IR *ir = b->ir();
    Literal *lv = this->ComplexFloat32(ir)->literal(PASSLOC, v);
    return _base->Const(PASSLOC, b, lv);
}

Value *
ComplexExtension::ConstComplexFloat64(LOCATION, Builder *b, std::complex<double> v) {
    IR *ir = b->ir();
    Literal *lv = this->ComplexFloat64(ir)->literal(PASSLOC, v);
    return _base->Const(PASSLOC, b, lv);
}

//
// Complex_BaseExtensionChecker
//
bool
Complex_BaseExtensionChecker::validateAdd(LOCATION, Builder *b, Value *left, Value *right) {
    const Type *leftType = left->type();
    const Type *rightType = right->type();
    
    // Check if either operand is a Complex type
    if (leftType->isKind<ComplexType>() || rightType->isKind<ComplexType>()) {
        // Both must be Complex types with the same element type
        if (leftType->isKind<ComplexType>() && rightType->isKind<ComplexType>()) {
            const ComplexType *leftComplex = static_cast<const ComplexType *>(leftType);
            const ComplexType *rightComplex = static_cast<const ComplexType *>(rightType);
            if (leftComplex->elementType() == rightComplex->elementType()) {
                return true;
            }
        }
        failValidateAdd(PASSLOC, b, left, right);
    }
    return false;
}

void
Complex_BaseExtensionChecker::failValidateAdd(LOCATION, Builder *b, Value *left, Value *right) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_Add);
    const Type *lType = left->type();
    const Type *rType = right->type();
    Allocator *mem = _base->compiler()->mem();
    e.setMessageLine(String(mem, "Complex Add: invalid input types (both operands must be same complex type)"))
     .appendMessageLine(String(mem, "    left ").append(lType->to_string(mem)))
     .appendMessageLine(String(mem, "   right ").append(rType->to_string(mem)));
    throw e;
}

bool
Complex_BaseExtensionChecker::validateMul(LOCATION, Builder *b, Value *left, Value *right) {
    const Type *leftType = left->type();
    const Type *rightType = right->type();
    
    // Check if either operand is a Complex type
    if (leftType->isKind<ComplexType>() || rightType->isKind<ComplexType>()) {
        // Both must be Complex types with the same element type
        if (leftType->isKind<ComplexType>() && rightType->isKind<ComplexType>()) {
            const ComplexType *leftComplex = static_cast<const ComplexType *>(leftType);
            const ComplexType *rightComplex = static_cast<const ComplexType *>(rightType);
            if (leftComplex->elementType() == rightComplex->elementType()) {
                return true;
            }
        }
        failValidateMul(PASSLOC, b, left, right);
    }
    return false;
}

void
Complex_BaseExtensionChecker::failValidateMul(LOCATION, Builder *b, Value *left, Value *right) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_Mul);
    const Type *lType = left->type();
    const Type *rType = right->type();
    Allocator *mem = _base->compiler()->mem();
    e.setMessageLine(String(mem, "Complex Mul: invalid input types (both operands must be same complex type)"))
     .appendMessageLine(String(mem, "    left ").append(lType->to_string(mem)))
     .appendMessageLine(String(mem, "   right ").append(rType->to_string(mem)));
    throw e;
}

bool
Complex_BaseExtensionChecker::validateSub(LOCATION, Builder *b, Value *left, Value *right) {
    const Type *leftType = left->type();
    const Type *rightType = right->type();
    
    // Check if either operand is a Complex type
    if (leftType->isKind<ComplexType>() || rightType->isKind<ComplexType>()) {
        // Both must be Complex types with the same element type
        if (leftType->isKind<ComplexType>() && rightType->isKind<ComplexType>()) {
            const ComplexType *leftComplex = static_cast<const ComplexType *>(leftType);
            const ComplexType *rightComplex = static_cast<const ComplexType *>(rightType);
            if (leftComplex->elementType() == rightComplex->elementType()) {
                return true;
            }
        }
        failValidateSub(PASSLOC, b, left, right);
    }
    return false;
}

void
Complex_BaseExtensionChecker::failValidateSub(LOCATION, Builder *b, Value *left, Value *right) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_Sub);
    const Type *lType = left->type();
    const Type *rType = right->type();
    Allocator *mem = _base->compiler()->mem();
    e.setMessageLine(String(mem, "Complex Sub: invalid input types (both operands must be same complex type)"))
     .appendMessageLine(String(mem, "    left ").append(lType->to_string(mem)))
     .appendMessageLine(String(mem, "   right ").append(rType->to_string(mem)));
    throw e;
}

} // namespace Complex
} // namespace JB2
} // namespace OMR
