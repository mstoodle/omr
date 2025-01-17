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
#include "ComplexOperations.hpp"
#include "Compiler.hpp"
#include "Extension.hpp"
#include "JB1CodeGenerator.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "Strategy.hpp"
#include "TextWriter.hpp"
#include "TypeReplacer.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {
namespace Complex {

INIT_JBALLOC_REUSECAT(ComplexExtension, Extension)

const SemanticVersion ComplexExtension::version(COMPLEXEXT_MAJOR,COMPLEXEXT_MINOR,COMPLEXEXT_PATCH);
const SemanticVersion ComplexExtension::requiredBaseVersion(REQUIRED_BASEEXT_MAJOR,REQUIRED_BASEEXT_MINOR,REQUIRED_BASEEXT_PATCH);
const String ComplexExtension::NAME("complex");

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Allocator *mem = compiler->mem();
        return new (mem) ComplexExtension(MEM_PASSLOC(mem), compiler);
    }
}

ComplexExtension::ComplexExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(a, compiler, (extended ? extensionName : NAME))
    , _base(compiler->loadExtension<Base::BaseExtension>(PASSLOC, &requiredBaseVersion))
    , ComplexFloat32(new ComplexFloat32Type(PASSLOC, this, _base->Float32))
    , ComplexFloat64(new ComplexFloat64Type(PASSLOC, this, _base->Float64))
    , aReal(registerAction(String("Real")))
    , aImag(registerAction(String("Imag")))
    , aConjugate(registerAction(String("Conjugate")))
    , aMagnitude(registerAction(String("Magnitude")))
    , CompileFail_BadInputTypes_Real(registerReturnCode("CompileFail_BadInputTypes_Real"))
    , CompileFail_BadInputTypes_Imag(registerReturnCode("CompileFail_BadInputTypes_Imag"))
    , CompileFail_BadInputTypes_Conjugate(registerReturnCode("CompileFail_BadInputTypes_Conjugate"))
    , CompileFail_BadInputTypes_Magnitude(registerReturnCode("CompileFail_BadInputTypes_Magitude")) {

    Strategy *jb1cgStrategy = new Strategy(compiler, "jb1cg");
    TypeReplacer *replacer = (new TypeReplacer(compiler))
                                  ->explode(ComplexFloat32)
                                  ->explode(ComplexFloat64);
    jb1cgStrategy->addPass(replacer);
    Pass *jb1cg = new JB1CodeGenerator(compiler);
    jb1cgStrategy->addPass(jb1cg);
    _jb1cgStrategyID = jb1cgStrategy->id();

    if (_base != NULL)
        _base->registerChecker(new Complex_BaseExtensionChecker(this, _base));
}

ComplexExtension::~ComplexExtension() {
    delete ComplexFloat64;
    delete ComplexFloat32;
}

//
// Operations
//
bool
ComplexExtensionChecker::validateReal(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (type == _xc->ComplexFloat32 && type != _xc->ComplexFloat64) {
        failValidateReal(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateReal(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Real);
    e.setMessageLine(String("Real: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string()))
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
    addOperation(b, new Op_Real(PASSLOC, this, b, aReal, result, value));
    return result;
}

bool
ComplexExtensionChecker::validateImag(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (type == _xc->ComplexFloat32 && type != _xc->ComplexFloat64) {
        failValidateImag(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateImag(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Imag);
    e.setMessageLine(String("Imag: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string()))
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
    addOperation(b, new Op_Imag(PASSLOC, this, b, aReal, result, value));
    return result;
}

bool
ComplexExtensionChecker::validateConjugate(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (type == _xc->ComplexFloat32 && type != _xc->ComplexFloat64) {
        failValidateConjugate(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateConjugate(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Conjugate);
    e.setMessageLine(String("Conjugate: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string()))
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
    addOperation(b, new Op_Conjugate(PASSLOC, this, b, aReal, result, value));
    return result;
}

bool
ComplexExtensionChecker::validateMagnitude(LOCATION, Builder *b, Value *value) {
    const Type *type = value->type();
    if (type == _xc->ComplexFloat32 && type != _xc->ComplexFloat64) {
        failValidateMagnitude(PASSLOC, b, value);
    }

    return true;
}

void
ComplexExtensionChecker::failValidateMagnitude(LOCATION, Builder *b, Value *value) {
    CompilationException e(PASSLOC, _xc->compiler(), _xc->CompileFail_BadInputTypes_Magnitude);
    e.setMessageLine(String("Magnitude: invalid input type"))
     .appendMessageLine(String("   value ").append(value->type()->to_string()))
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
    addOperation(b, new Op_Magnitude(PASSLOC, this, b, aMagnitude, result, value));
    return result;
}

//
// Pseudo operations
//
Value *
ComplexExtension::ConstComplexFloat32(LOCATION, Builder *b, std::complex<float> v)
   {
   Literal *lv = this->ComplexFloat32->literal(PASSLOC, b->comp(), v);
   return _base->Const(PASSLOC, b, lv);
   }

Value *
ComplexExtension::ConstComplexFloat64(LOCATION, Builder *b, std::complex<double> v)
   {
   Literal *lv = this->ComplexFloat64->literal(PASSLOC, b->comp(), v);
   return _base->Const(PASSLOC, b, lv);
   }

} // namespace Complex
} // namespace JB2
} // namespace OMR
