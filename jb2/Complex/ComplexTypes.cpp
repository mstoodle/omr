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
#include "ComplexTypes.hpp"
#include "Compiler.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "TextWriter.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {
namespace Complex {

TypeKind ComplexType::TYPEKIND = KindService::NoKind;;
bool ComplexType::kindRegistered = false;

ComplexType::ComplexType(LOCATION, TypeKind kind, ComplexExtension *cext, const Base::NumericType *elementType)
    : NumericType(PASSLOC,
                  kind,
                  cext,
                  String("Complex<").append(elementType->name()).append(String(">")),
                  elementType->size())
    , _elementType(elementType) {
}

const TypeKind
ComplexType::getTypeClassKind() {
    if (!kindRegistered) {
        TYPEKIND = Type::kindService.assignKind(Base::NumericType::getTypeClassKind(), "ComplexType");
        kindRegistered = true;
    }
    return TYPEKIND;
}

ComplexExtension *
ComplexType::xc() {
    return static_cast<ComplexExtension *>(_ext);
}

bool
ComplexType::registerJB1Type(JB1MethodBuilder *j1mb) const {
    CompilationException e(LOC, ext()->compiler(), ext()->compiler()->CompileFail_TypeMustBeReduced);
    e.setMessage(String("registerJB1Type: ComplexTypes must be reduced before jb1codegen"));
    throw e;
}

void
ComplexType::createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const {
    CompilationException e(LOC, ext()->compiler(), ext()->compiler()->CompileFail_TypeMustBeReduced);
    e.setMessage(String("createJB1ConstOp: ComplexTypes must be reduced before jb1codegen"))
     .appendMessage(String("    in builder B").append(String::to_string(b->id())))
     .appendMessage(String("    producing result v").append(String::to_string(result->id())))
     .appendMessage(String("    for literal lv").append(String::to_string(lv->id())));
    throw e;
}


template <typename T> TypeKind ComplexFloatType<T>::TYPEKIND = KindService::NoKind;;
template <typename T> bool ComplexFloatType<T>::kindRegistered = false;

#define str(s) #s
template <typename T>
const TypeKind
ComplexFloatType<T>::getTypeClassKind() {
    if (!kindRegistered) {
        TYPEKIND = Type::kindService.assignKind(ComplexType::getTypeClassKind(), "ComplexFloatType<" str(T) ">");
        kindRegistered = true;
    }
    return TYPEKIND;
}

template <typename T>
Literal *
ComplexFloatType<T>::literal(LOCATION, Compilation *comp, const std::complex<T> value) const {
    std::complex<T> *pValue = new std::complex<T>[1];
    *pValue = value;
    return this->Type::literal(PASSLOC, comp, reinterpret_cast<const LiteralBytes *>(pValue));
}

template<typename T>
bool
ComplexFloatType<T>::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const std::complex<T> *>(l1)) == (*reinterpret_cast<const std::complex<T> *>(l2));
}

template<typename T>
void
ComplexFloatType<T>::printValue(TextWriter &w, const void *p) const
   {
   std::complex<T> value = *(reinterpret_cast<const 
   jjjjjtd::complex<T> *>(p));
   w << name() << " (" << value.real() << "+i" << value.imag() << ")";
   }

template<typename T>
void
ComplexFloatType<T>::printLiteral(TextWriter & w, const Literal *lv) const {
   std::complex<T> value = lv->value<const std::complex<T> >();
   w << name() << "(" << value.real() << "+i" << value.imag() << ")";
}

template<typename T>
const T
ComplexFloatType<T>::real(const Literal *lv) const {
    std::complex<T> value = lv->value<const std::complex<T> >();
    return (const T) value.real();
}

template<typename T>
const T
ComplexFloatType<T>::imag(const Literal *lv) const {
    std::complex<T> value = lv->value<const std::complex<T> >();
    return (const T) value.imag();
}

template class ComplexFloatType<float>;
template class ComplexFloatType<double>;

} // namespace Complex
} // namespace JB2
} // namespace OMR
