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

//#define DO_LOGGING true
#define DO_LOGGING false

#include "ComplexMatMult.hpp"
#include "jb2cg/omrgen.hpp"

using namespace OMR::JB2;

ComplexMatMult::ComplexMatMult(MEM_LOCATION(a), Compiler * compiler, Complex::ComplexExtension *cx)
    : Func::Function(MEM_PASSLOC(a), compiler, "ComplexMatMult")
    , _cx(cx)
    , _bx(cx->base()) {

    DefineName("complexmatmult");
    DefineFile("ComplexMatMult.cpp");
    DefineLine("46");
}

bool
ComplexMatMult::buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *fc) {
    CoreExtension *coreExt = comp->compiler()->loadExtension<CoreExtension>(PASSLOC);
    IR *ir = comp->ir();
    pComplex = _bx->PointerTo(PASSLOC, _cx->ComplexFloat64(ir));

    _sumVar = fc->DefineLocal("sum", _cx->ComplexFloat64(ir));
    _paramC = fc->DefineParameter("C", pComplex);
    _paramA = fc->DefineParameter("A", pComplex);
    _paramB = fc->DefineParameter("B", pComplex);
    _paramN = fc->DefineParameter("N", _bx->Int64(ir));

    fc->DefineReturnType(coreExt->NoType(ir));

    return true;
}


Value *
ComplexMatMult::elementAt(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N) {

    Value *rowStart = _bx->Mul(PASSLOC, bldr, first, N);
    Value *elementIndex = _bx->Add(PASSLOC, bldr, rowStart, second);
    Value *elementPointer = _bx->IndexAt(PASSLOC, bldr, base, elementIndex);
    return elementPointer;
}

void
ComplexMatMult::Store2D(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N, Value *value) {
    _bx->StoreAt(PASSLOC, bldr, elementAt(PASSLOC, bldr, base, first, second, N), value);
}

Value *
ComplexMatMult::Load2D(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N) {
    return _bx->LoadAt(PASSLOC, bldr, elementAt(PASSLOC, bldr, base, first, second, N));
}

bool
ComplexMatMult::buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *fc) {
    Func::FunctionExtension *fx = comp->compiler()->loadExtension<Func::FunctionExtension>(PASSLOC);
    Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
    IR *ir = comp->ir();

    Func::LocalSymbol *iVar = fc->DefineLocal("i", _bx->Int64(ir));
    Func::LocalSymbol *jVar = fc->DefineLocal("j", _bx->Int64(ir));
    Func::LocalSymbol *kVar = fc->DefineLocal("k", _bx->Int64(ir));

    Value *A = fx->Load(LOC, entry, _paramA);
    Value *B = fx->Load(LOC, entry, _paramB);
    Value *C = fx->Load(LOC, entry, _paramC);
    Value *N = fx->Load(LOC, entry, _paramN);
    Value *zero = _bx->ConstInt64(LOC, entry, 0);
    Value *one = _bx->ConstInt64(LOC, entry, 1);

    Base::ForLoopBuilder iLoop = _bx->ForLoopUp(LOC, entry, iVar, zero, N, one); {
        Builder *iBody = iLoop.loopBody();
        Value *i = fx->Load(LOC, iBody, iVar);

        Base::ForLoopBuilder jLoop = _bx->ForLoopUp(LOC, iBody, jVar, zero, N, one); {
            Builder *jBody = jLoop.loopBody();
            Value *j = fx->Load(LOC, jBody, jVar);

            std::complex<double> complexZero(0.0, 0.0);
            fx->Store(LOC, jBody, _sumVar, _cx->ConstComplexFloat64(LOC, jBody, complexZero));

            Base::ForLoopBuilder kLoop = _bx->ForLoopUp(LOC, jBody, kVar, zero, N, one); {
                Builder *kBody = kLoop.loopBody();
                Value *k = fx->Load(LOC, kBody, kVar);

                Value *A_ik = Load2D(LOC, kBody, A, i, k, N);
                Value *B_kj = Load2D(LOC, kBody, B, k, j, N);
                Value *product = _bx->Mul(LOC, kBody, A_ik, B_kj);
                Value *sum = fx->Load(LOC, kBody, _sumVar);
                Value *newSum = _bx->Add(LOC, kBody, sum, product);
                fx->Store(LOC, kBody, _sumVar, newSum);
            }

            Store2D(LOC, jBody, C, i, j, N, fx->Load(LOC, jBody, _sumVar));
        }
    }

    fx->Return(LOC, entry);

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

    cout << "Step 1: load jb2\n";
    void *handle = dlopen(OMR_JB2_CORELIB, RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        return -1;
    }

    cout << "Step 2: create a Compiler\n";
    Compiler c("Compiler for Complex Matrix Multiply Code Sample");

    cout << "Step 3: load extensions (Base, Func, and Complex)\n";
    CoreExtension *coreExt = c.lookupExtension<CoreExtension>();
    Base::BaseExtension *bx = c.loadExtension<Base::BaseExtension>(LOC);
    Func::FunctionExtension *fx = c.loadExtension<Func::FunctionExtension>(LOC);
    Complex::ComplexExtension *cx = c.loadExtension<Complex::ComplexExtension>(LOC);
    omrgen::OMRExtension *ogx = c.loadExtension<omrgen::OMRExtension>(LOC);
    if (c.hasErrorCondition()) {
        cerr << c.errorCondition()->message().c_str();
        return -2;
    }
    assert(coreExt != NULL && bx != NULL && fx != NULL && cx != NULL && ogx != NULL);

    cout << "Step 4: Create Function object\n";
    ComplexMatMult cmmFunc(MEM_LOC(c.allocator()), &c, cx);

    cout << "Step 5: Set up logging configuration\n";
    TextLogger logger(std::cout, String("  "));
    TextLogger *wrt = (DO_LOGGING) ? &logger : NULL;
    if (DO_LOGGING) {
        c.config()->setTracePrototypeIR(true)
                  ->setTraceBuildIL(true)
                  ->setTraceStrategy(true)
                  ->setTraceCodeGenerator(true)
                  ->setTraceTypeReplacer(true)
                  ->setTraceVisitor(true)
                  ;
    }
    
    cout << "Step 6: compile function\n";
    CompiledBody *body = fx->compile(LOC, &cmmFunc, coreExt->strategyCodegen, wrt);

    if (body == NULL) {
        cout << "Compile failed\n";
        exit(-1);
    }
    
    ComplexMatMultFunctionType *cmm = body->nativeEntryPoint<ComplexMatMultFunctionType>(0);

    cout << "Matrix Multiply operands:\n";
    printMatrix(A, N, "A");
    printMatrix(B, N, "B");

    cmm(C, A, B, N);

    cout << "Result:\n";
    printMatrix(C, N, "C");
}
