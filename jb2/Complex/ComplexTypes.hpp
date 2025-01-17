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

#ifndef COMPLEXTYPES_INCL
#define COMPLEXTYPES_INCL

#include <complex>
#include <map>
#include "JBCore.hpp"
#include "Base/BaseTypes.hpp"


namespace OMR {
namespace JB2 {
namespace Complex {

class ComplexExtension;

class ComplexType : public Base::NumericType {
    friend class ComplexExtension;

public:
    const Type *elementType() const { return _elementType; }

    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;

    static const TypeKind getTypeClassKind();
protected: 
    ComplexType(LOCATION, TypeKind kind, ComplexExtension *cext, const Base::NumericType *elementType);
    ComplexExtension *xc();

    const Type *_elementType;

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

template <typename T>
class ComplexFloatType : public ComplexType {
    friend class ComplexExtension;

    public:
    virtual size_t size() const { return sizeof(std::complex<T>); }
    Literal *literal(LOCATION, Compilation *comp, const std::complex<T> value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, {0.0,0.0}); }
    Literal *identity(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, {1.0,0.0} ); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual bool isConcrete() const { return true; }
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual const T real(const Literal *lv) const;
    virtual const T imag(const Literal *lv) const;

    static const TypeKind getTypeClassKind();

    protected:
    ComplexFloatType<T>(LOCATION, ComplexExtension *cext, const Base::NumericType *elementType)
        : ComplexType(PASSLOC, TYPEKIND, cext, elementType) { }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

typedef ComplexFloatType<float> ComplexFloat32Type;
typedef ComplexFloatType<double> ComplexFloat64Type;

} // namespace Complex
} // namespace JB2
} // namespace OMR

#endif // defined(COMPLEXTYPES_INCL)
