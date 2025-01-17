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
    JBALLOC_(ComplexType)

public:
    const Type *elementType() const { return _elementType; }

protected:
    ComplexType(MEM_LOCATION(a), ExtensibleKind kind, ComplexExtension *cext, const Base::NumericType *elementType);
    ComplexType(Allocator *a, const ComplexType *source, IRCloner *cloner);

    ComplexExtension *xc();
    
    const Base::StructType *createLayout(LOCATION);

    const Type *_elementType;

    SUBCLASS_KINDSERVICE_DECL(Extensible,ComplexType);
};

template <typename T>
class ComplexFloatType : public ComplexType {
    friend class ComplexExtension;
    friend class ComplexIRAddon;
    JBALLOC_(ComplexFloatType)

public:
    
    virtual size_t size() const { return 8*sizeof(std::complex<T>); }
    Literal *literal(LOCATION, const std::complex<T> value) const;
    Literal *zero(LOCATION) const { return literal(PASSLOC, {0.0,0.0}); }
    Literal *identity(LOCATION) const { return literal(PASSLOC, {1.0,0.0} ); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual bool isConcrete() const { return true; }
    virtual void logValue(TextLogger &lgr, const void *p) const;
    virtual void logLiteral(TextLogger &lgr, const Literal *lv) const;
    virtual const T real(const Literal *lv) const;
    virtual const T imag(const Literal *lv) const;

protected:
    ComplexFloatType<T>(MEM_LOCATION(a), ComplexExtension *cext, const Base::NumericType *elementType);
    ComplexFloatType<T>(Allocator *a, const ComplexFloatType<T> *source, IRCloner *cloner);

    virtual const Type *cloneType(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Extensible,ComplexFloatType);
};

typedef ComplexFloatType<float> ComplexFloat32Type;
typedef ComplexFloatType<double> ComplexFloat64Type;

} // namespace Complex
} // namespace JB2
} // namespace OMR

#endif // defined(COMPLEXTYPES_INCL)
