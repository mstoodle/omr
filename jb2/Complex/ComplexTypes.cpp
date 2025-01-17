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

INIT_JBALLOC_REUSECAT(ComplexType, Type)
SUBCLASS_KINDSERVICE_IMPL(ComplexType, "ComplexType", Base::NumericType, Extensible);

const Base::StructType *
ComplexType::createLayout(LOCATION) {
    ComplexExtension *cext = ext()->refine<ComplexExtension>();
    Base::BaseExtension *base = cext->base();
    IR *ir = _elementType->ir();
    Allocator *mem = ir->allocator();
    
    // Validate that elementType is Float32 or Float64
    const Base::Float32Type *float32 = base->Float32(ir);
    const Base::Float64Type *float64 = base->Float64(ir);
    
    if (_elementType != float32 && _elementType != float64) {
        // Invalid element type for Complex, should throw an exception
        assert(0 && "Complex types can only be created with Float32 or Float64 element types");
        return NULL;
    }
    
    // Create a struct layout with two fields: real and imag
    Base::StructTypeBuilder *builder = new (mem) Base::StructTypeBuilder(base, ir);
    builder->setName(String(mem, "Complex<").append(_elementType->name()).append(String(mem, ">_layout")))
           ->addField(String(mem, "real"), _elementType, 0)
           ->addField(String(mem, "imag"), _elementType, _elementType->size())
           ->setSize(2 * _elementType->size());
    
    return builder->create(PASSLOC);
}

ComplexType::ComplexType(MEM_LOCATION(a), ExtensibleKind kind, ComplexExtension *cext, const Base::NumericType *elementType)
    : NumericType(MEM_PASSLOC(a),
                  kind,
                  cext,
                  elementType->ir(),
                  String(a, "Complex<").append(elementType->name()).append(String(a, ">")),
                  2 * elementType->size())
    , _elementType(elementType) {
    setLayout(createLayout(PASSLOC));
}

ComplexType::ComplexType(Allocator *a, const ComplexType *source, IRCloner *cloner)
    : NumericType(a, source, cloner)
    , _elementType(cloner->clonedType(source->elementType())->template refine<Base::NumericType>()) {
    assert(source->layout() != NULL);
    setLayout(cloner->clonedType(source->layout()));
}

ComplexType::~ComplexType(void) {

}

ComplexExtension *
ComplexType::xc() {
    return static_cast<ComplexExtension *>(_ext);
}

template <typename T>
ComplexFloatType<T>::ComplexFloatType(MEM_LOCATION(a), ComplexExtension *cext, const Base::NumericType *elementType)
    : ComplexType(MEM_PASSLOC(a), getExtensibleClassKind(), cext, elementType) {
}

template <typename T>
ComplexFloatType<T>::ComplexFloatType(Allocator *a, const ComplexFloatType<T> *source, IRCloner *cloner)
    : ComplexType(a, source, cloner) {
}

template <typename T>
const Type *
ComplexFloatType<T>::cloneType(Allocator *a, IRCloner *cloner) const {
    return new (a) ComplexFloatType<T>(a, this, cloner);
}

template <typename T>
Literal *
ComplexFloatType<T>::literal(LOCATION, const std::complex<T> value) const {
    std::complex<T> *pValue = ir()->allocator()->template allocate<std::complex<T>>(1);
    *pValue = value;
    return this->Type::literal(PASSLOC, reinterpret_cast<const LiteralBytes *>(pValue));
}

template<typename T>
bool
ComplexFloatType<T>::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const std::complex<T> *>(l1)) == (*reinterpret_cast<const std::complex<T> *>(l2));
}

template<typename T>
void
ComplexFloatType<T>::logValue(TextLogger &lgr, const void *p) const {
    std::complex<T> value = *(reinterpret_cast<const std::complex<T> *>(p));
    lgr << name() << " (";
    lgr << value.real();
    if (value.imag() < 0) {
        lgr << " - i" << -value.imag();
    } else {
        lgr << " + i" << value.imag();
    }
    lgr << ")";
}

template<typename T>
void
ComplexFloatType<T>::logLiteral(TextLogger &lgr, const Literal *lv) const {
   std::complex<T> value = lv->value<const std::complex<T> >();
   lgr << name() << "(";
   lgr << value.real();
   if (value.imag() < 0) {
       lgr << " - i" << -value.imag();
   } else {
       lgr << " + i" << value.imag();
   }
   lgr << ")";
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

// Template instantiations
template<> AllocationCategoryID ComplexFloatType<float>::allocCategory = NoAllocationCategory;
template<> bool ComplexFloatType<float>::allocCategoryInitialized = false;
template<> AllocationCategoryID ComplexFloatType<float>::allocCat() {
    if (!allocCategoryInitialized) {
        allocCategory = Type::allocCat();
        allocCategoryInitialized = true;
    }
    return allocCategory;
}
template<> ExtensibleKind ComplexFloatType<float>::ComplexFloatTypeExtensibleKind = KindService::NoKind;
template<> bool ComplexFloatType<float>::kindRegistered = false;
template<> const ExtensibleKind ComplexFloatType<float>::getExtensibleClassKind() {
    if (!kindRegistered) {
        ComplexFloatTypeExtensibleKind = Type::kindService.assignKind(ComplexType::getExtensibleClassKind(), String("ComplexFloat32Type"));
        kindRegistered = true;
    }
    return ComplexFloatTypeExtensibleKind;
}
template class ComplexFloatType<float>;

template<> AllocationCategoryID ComplexFloatType<double>::allocCategory = NoAllocationCategory;
template<> bool ComplexFloatType<double>::allocCategoryInitialized = false;
template<> AllocationCategoryID ComplexFloatType<double>::allocCat() {
    if (!allocCategoryInitialized) {
        allocCategory = Type::allocCat();
        allocCategoryInitialized = true;
    }
    return allocCategory;
}
template<> ExtensibleKind ComplexFloatType<double>::ComplexFloatTypeExtensibleKind = KindService::NoKind;
template<> bool ComplexFloatType<double>::kindRegistered = false;
template<> const ExtensibleKind ComplexFloatType<double>::getExtensibleClassKind() {
    if (!kindRegistered) {
        ComplexFloatTypeExtensibleKind = Type::kindService.assignKind(ComplexType::getExtensibleClassKind(), String("ComplexFloat64Type"));
        kindRegistered = true;
    }
    return ComplexFloatTypeExtensibleKind;
}
template class ComplexFloatType<double>;

template<> ComplexFloatType<float>::~ComplexFloatType() { }
template<> ComplexFloatType<double>::~ComplexFloatType() { }

} // namespace Complex
} // namespace JB2
} // namespace OMR
