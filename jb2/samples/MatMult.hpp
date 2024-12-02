/*******************************************************************************
 * Copyright (c) 2016, 2022 IBM Corp.
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/


#ifndef MATMULT_INCL
#define MATMULT_INCL

#include "Base/BaseExtension.hpp"
#include "Base/Function.hpp"

typedef void (FloatMatMultFunctionType)(float *, float *, float *, int32_t);
typedef void (DoubleMatMultFunctionType)(double *, double *, double *, int32_t);
#if 0
typedef void (ComplexFloatMatMultFunctionType)(std::complex<double> *, std::complex<double> *, std::complex<double> *, int32_t);
typedef void (ComplexDoubleMatMultFunctionType)(std::complex<float> *, std::complex<float> *, std::complex<float> *, int32_t);
#endif

using namespace OMR::JitBuilder;

class MatMult : public Base::Function {
    public:
    MatMult(Compiler *compiler, Base::BaseExtension *base, const Type *elementType);
    virtual bool buildIL();

    protected:
    Base::BaseExtension *_base;
    const Type *_elementType;
    const Type *_pElementType;
    Base::ParameterSymbol *_symA;
    Base::ParameterSymbol *_symB;
    Base::ParameterSymbol *_symC;
    Base::ParameterSymbol *_symN;
    Base::LocalSymbol *_symSum;

    void Store2D(LOCATION,
                 Builder *b,
                 Value *base,
                 Value *first,
                 Value *second,
                 Value *N,
                 Value *value);
    Value *Load2D(LOCATION,
                  Builder *b,
                  Value *base,
                  Value *first,
                  Value *second,
                  Value *N);
};

#endif // !defined(MATMULT_INCL)
