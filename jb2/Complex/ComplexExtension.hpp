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

#ifndef COMPLEXEXTENSION_INCL
#define COMPLEXEXTENSION_INCL

#include <complex>
#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Func/Func.hpp"
#include "ComplexTypes.hpp"


namespace OMR {
namespace JB2 {
namespace Complex {

class ComplexExtensionChecker;

class ComplexExtension : public Extension {
    JBALLOC_(ComplexExtension)

public:
    ComplexExtension(MEM_LOCATION(a), Compiler *compiler, bool extended=false, String extensionName="");

    static const String NAME;

    virtual const SemanticVersion * semver() const {
        return &version;
    }

protected: // need to initialize this first so put it up here
    Base::BaseExtension *_base;

public:
    Base::BaseExtension *base() { return _base; }

    //
    // Complex Types
    //

    const ComplexFloat32Type *ComplexFloat32;
    const ComplexFloat64Type *ComplexFloat64;


    //
    // Actions
    //

    const ActionID aReal;
    const ActionID aImag;
    const ActionID aConjugate;
    const ActionID aMagnitude;

    //
    // CompileResults
    //

    const CompilerReturnCode CompileFail_BadInputTypes_Real;
    const CompilerReturnCode CompileFail_BadInputTypes_Imag;
    const CompilerReturnCode CompileFail_BadInputTypes_Conjugate;
    const CompilerReturnCode CompileFail_BadInputTypes_Magnitude;


    //
    // Operations
    //

    Value * Real(LOCATION, Builder *b, Value *v);
    Value * Imag(LOCATION, Builder *b, Value *v);
    Value * Conjugate(LOCATION, Builder *b, Value *v);
    Value * Magnitude(LOCATION, Builder *b, Value *v);


    //
    // Pseudo operations
    //

    Value * ConstComplexFloat32(LOCATION, Builder *b, std::complex<float> v);
    Value * ConstComplexFloat64(LOCATION, Builder *b, std::complex<double> v);

    void registerChecker(ComplexExtensionChecker *checker);

    protected:
    StrategyID _jb1cgStrategyID;

    List<ComplexExtensionChecker *> _checkers;

    static const MajorID COMPLEXEXT_MAJOR=0;
    static const MinorID COMPLEXEXT_MINOR=1;
    static const PatchID COMPLEXEXT_PATCH=0;
    static const SemanticVersion version;

    static const MajorID REQUIRED_BASEEXT_MAJOR=0;
    static const MinorID REQUIRED_BASEEXT_MINOR=1;
    static const PatchID REQUIRED_BASEEXT_PATCH=0;
    static const SemanticVersion requiredBaseVersion;
};

class Complex_BaseExtensionChecker : public Base::BaseExtensionChecker {
public:
    Complex_BaseExtensionChecker(ComplexExtension *xc, Base::BaseExtension *base)
        : Base::BaseExtensionChecker(base)
        , _xc(xc) {
    }

    virtual bool validateAdd(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateMul(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateSub(LOCATION, Builder *b, Value *left, Value *right);

protected:
    virtual void failValidateAdd(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateMul(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateSub(LOCATION, Builder *b, Value *left, Value *right);

    ComplexExtension *_xc;
};

class ComplexExtensionChecker {
public:
    ComplexExtensionChecker(ComplexExtension *xc)
        : _xc(xc) {
    }

    virtual bool validateReal(LOCATION, Builder *b, Value *value);
    virtual bool validateImag(LOCATION, Builder *b, Value *value);
    virtual bool validateConjugate(LOCATION, Builder *b, Value *value);
    virtual bool validateMagnitude(LOCATION, Builder *b, Value *value);

protected:
    virtual void failValidateReal(LOCATION, Builder *b, Value *value);
    virtual void failValidateImag(LOCATION, Builder *b, Value *value);
    virtual void failValidateConjugate(LOCATION, Builder *b, Value *value);
    virtual void failValidateMagnitude(LOCATION, Builder *b, Value *value);

    ComplexExtension *_xc;
};

} // namespace Complex
} // namespace JB2
} // namespace OMR

#endif // defined(COMPLEXEXTENSION_INCL)
