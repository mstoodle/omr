/*******************************************************************************
 * Copyright (c) 2016, 2020 IBM Corp. and others
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


#include <complex>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#define DO_LOGGING 1
// #define DO_LOGGING 0

#include "ComplexMatMult.hpp"

using namespace OMR::JB2;

ComplexMatMult::ComplexMatMult(LOCATION, Compiler * compiler, Complex::ComplexExtension *xc)
    : Base::Function(PASSLOC, compiler)
    , _xc(xc)
    , _base(xc->base()) {

    DefineName("complexmatmult");
}

bool
ComplexMatMult::buildContext(LOCATION, Base::FunctionCompilation *comp, Base::FunctionContext *fc) {
    pComplex = _base->PointerTo(PASSLOC, comp, _xc->ComplexFloat64);

    _sumVar = fc->DefineLocal("sum", _xc->ComplexFloat64);
    _paramC = fc->DefineParameter("C", pComplex);
    _paramA = fc->DefineParameter("A", pComplex);
    _paramB = fc->DefineParameter("B", pComplex);
    _paramN = fc->DefineParameter("N", pComplex);

    fc->DefineReturnType(_base->NoType);

    return true;
}


Value *
ComplexMatMult::elementAt(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N) {

    Value *rowStart = _base->Mul(PASSLOC, bldr, first, N);
    Value *elementIndex = _base->Add(PASSLOC, bldr, rowStart, second);
    Value *elementPointer = _base->IndexAt(PASSLOC, bldr, base, elementIndex);
    return elementPointer;
}

void
ComplexMatMult::Store2D(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N, Value *value) {
    _base->StoreAt(PASSLOC, bldr, elementAt(PASSLOC, bldr, base, first, second, N), value);
}

Value *
ComplexMatMult::Load2D(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N) {
    return _base->LoadAt(PASSLOC, bldr, elementAt(PASSLOC, bldr, base, first, second, N));
}

bool
ComplexMatMult::buildIL(LOCATION, Base::FunctionCompilation *comp, Base::FunctionContext *fc) {
    Builder *entry = fc->builderEntryPoint();

    Base::LocalSymbol *iVar = fc->DefineLocal("i", _base->Int64);
    Base::LocalSymbol *jVar = fc->DefineLocal("j", _base->Int64);
    Base::LocalSymbol *kVar = fc->DefineLocal("k", _base->Int64);

    Value *A = _base->Load(LOC, entry, _paramA);
    Value *B = _base->Load(LOC, entry, _paramB);
    Value *C = _base->Load(LOC, entry, _paramC);
    Value *N = _base->Load(LOC, entry, _paramN);
    Value *zero = _base->ConstInt64(LOC, entry, 0);
    Value *one = _base->ConstInt64(LOC, entry, 1);

    Base::ForLoopBuilder *iLoop = _base->ForLoopUp(LOC, entry, iVar, zero, N, one); {
        Builder *iBody = iLoop->loopBody();
        Value *i = _base->Load(LOC, iBody, iVar);

        Base::ForLoopBuilder *jLoop = _base->ForLoopUp(LOC, iBody, jVar, zero, N, one); {
            Builder *jBody = jLoop->loopBody();
            Value *j = _base->Load(LOC, jBody, jVar);

            std::complex<double> complexZero(0.0, 0.0);
            _base->Store(LOC, jBody, _sumVar, _xc->ConstComplexFloat64(LOC, jBody, complexZero));

            Base::ForLoopBuilder *kLoop = _base->ForLoopUp(LOC, jBody, kVar, zero, N, one); {
                Builder *kBody = kLoop->loopBody();
                Value *k = _base->Load(LOC, kBody, kVar);

                Value *A_ik = Load2D(LOC, kBody, A, i, k, N);
                Value *B_kj = Load2D(LOC, kBody, B, k, j, N);
                Value *product = _base->Mul(LOC, kBody, A_ik, B_kj);
                _base->Increment(LOC, kBody, _sumVar, product);
            }

            Store2D(LOC, jBody, C, i, j, N, _base->Load(LOC, jBody, _sumVar));
        }
    }

    _base->Return(LOC, entry);

    return true;
}


void
printMatrix(std::complex<double> *M, int64_t N, const char *name) {
    printf("%s = [\n", name);
    for (int64_t i=0;i < N;i++) {
        printf("      [ (%lf,%lf)", M[i*N].real(), M[i*N].imag());
        for (int64_t j=1;j < N;j++)
            printf(", (%lf,%lf)", M[i * N + j].real(), M[i * N + j].imag());
        printf(" ],\n");
    }
    printf("    ]\n\n");
}

using namespace std;

int
main(int argc, char *argv[]) {
    cout << "Step 0: prepare input matrices\n";
    const int64_t N=4;
    std::complex<double> A[N*N];
    std::complex<double> B[N*N];
    std::complex<double> C[N*N];
    std::complex<double> D[N*N];
    for (int64_t i=0;i < N;i++) {
        for (int64_t j=0;j < N;j++) {
            A[i*N+j] = std::complex<double>(1.0,0.0);
            B[i*N+j] = std::complex<double>((double)i,(double)j);
            C[i*N+j] = std::complex<double>(0.0,0.0);
            D[i*N+j] = std::complex<double>(0.0,0.0);
        }
    }

    cout << "Step 1: load jb2.so\n";
    void *handle = dlopen("libjb2.so", RTLD_LAZY);
    if (!handle) {
        cerr << dlerror() << "\n";
        return -1;
    }

    cout << "Step 2: create a Compiler\n";
    Compiler c("Compiler for Complex Matrix Multiply Code Sample");

    cout << "Step 3: load extensions (Base and VM)\n";
    Base::BaseExtension *base = c.loadExtension<Base::BaseExtension>(LOC);
    Complex::ComplexExtension *xc = c.loadExtension<Complex::ComplexExtension>(LOC);
    if (c.hasErrorCondition()) {
        cerr << c.errorCondition()->message();
        return -2;
    }
    assert(xc != NULL);

    cout << "Step 4: Create Function object\n";
    ComplexMatMult cmmFunc(LOC, &c, xc);

    cout << "Step 5: Set up logging configuration\n";
    TextWriter logger(&c, std::cout, String("    "));
    TextWriter *log = (DO_LOGGING) ? &logger : NULL;
    
    printMatrix(A, N, "A");
    printMatrix(B, N, "B");

#if 0
    ComplexMatMultFunctionType *cmm = cmmFunc.debugEntry<ComplexMatMultFunctionType *>();
#else
    cout << "Step 6: compile function\n";
    CompilerReturnCode result = cmmFunc.compile(LOC, base->jb1cgStrategyID, log);

    if (result != c.CompileSuccessful) {
        cout << "Compile failed: " << result << "\n";
        exit(-1);
    }
    
    ComplexMatMultFunctionType *cmm = cmmFunc.compiledBody(base->jb1cgStrategyID)->nativeEntryPoint<ComplexMatMultFunctionType>();
#endif

    cout << "Matrix Multiply operands:\n";
    printMatrix(A, N, "A");
    printMatrix(B, N, "B");

    cmm(C, A, B, N);

    cout << "Result:\n";
    printMatrix(C, N, "C");
}
