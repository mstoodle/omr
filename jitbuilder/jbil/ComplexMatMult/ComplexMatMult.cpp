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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string>
#include <complex>

#include "complex.hpp"
#include "ComplexMatMult.hpp"
#include "ComplexPrettyPrinter.hpp"
#include "ComplexReducer.hpp"
#include "ComplexTypeDictionary.hpp"
#include "AppendBuilderInliner.hpp"


ComplexMatMult::ComplexMatMult(ComplexTypeDictionary * types)
   : OMR::JitBuilder::FunctionBuilder(types)
   , Complex(types->Complex)
   , pComplex(types->PointerTo(Complex))
   {
   DefineLine(LINETOSTR(__LINE__));
   DefineFile(__FILE__);

   DefineName("complexmatmult");

   // C = A * B, all NxN matrices
   DefineParameter("C", pComplex);
   DefineParameter("A", pComplex);
   DefineParameter("B", pComplex);
   DefineParameter("N", Int64);

   DefineReturnType(NoType);

   DefineLocal("sum", Complex);
   }


void
ComplexMatMult::Store2D(OMR::JitBuilder::Builder *bldr,
                        OMR::JitBuilder::Value *base,
                        OMR::JitBuilder::Value *first,
                        OMR::JitBuilder::Value *second,
                        OMR::JitBuilder::Value *N,
                        OMR::JitBuilder::Value *value)
   {
   bldr->StoreAt(
   bldr->   IndexAt(pComplex,
               base,
   bldr->      Add(
   bldr->         Mul(
                    first,
                    N),
                 second)),
            value);
   }

OMR::JitBuilder::Value *
ComplexMatMult::Load2D(OMR::JitBuilder::Builder *bldr,
                       OMR::JitBuilder::Value *base,
                       OMR::JitBuilder::Value *first,
                       OMR::JitBuilder::Value *second,
                       OMR::JitBuilder::Value *N)
   {
   return
      bldr->LoadAt(pComplex,
      bldr->   IndexAt(pComplex,
                  base,
      bldr->      Add(
      bldr->         Mul(
                       first,
                       N),
                     second)));
   }

bool
ComplexMatMult::buildIL()
   {
   OMR::JitBuilder::Value *A = Load("A");
   OMR::JitBuilder::Value *B = Load("B");
   OMR::JitBuilder::Value *C = Load("C");
   OMR::JitBuilder::Value *N = Load("N");
   OMR::JitBuilder::Value *zero = ConstInt64(0);
   OMR::JitBuilder::Value *one = ConstInt64(1);

   OMR::JitBuilder::Builder *iloop=OrphanBuilder();
   ForLoopUp("i", iloop, zero, N, one);
      {
      OMR::JitBuilder::Value *i = iloop->Load("i");

      OMR::JitBuilder::Builder *jloop = iloop->OrphanBuilder();
      iloop->ForLoopUp("j", jloop, zero, N, one);
         {
         OMR::JitBuilder::Value *j = jloop->Load("j");

         jloop->Store("sum",
         jloop->   ConstComplex(complex<double>(0.0,0.0)));

         OMR::JitBuilder::Builder *kloop = jloop->OrphanBuilder();
         jloop->ForLoopUp("k", kloop, zero, N, one);
            {
            OMR::JitBuilder::Value *k = kloop->Load("k");

            OMR::JitBuilder::Value *A_ik = Load2D(kloop, A, i, k, N);
            OMR::JitBuilder::Value *B_kj = Load2D(kloop, B, k, j, N);
            kloop->Store("sum",
            kloop->   Add(
            kloop->      Load("sum"),
            kloop->      Mul(A_ik, B_kj)));
            }

         Store2D(jloop, C, i, j, N, jloop->Load("sum"));
         }
      }

   Return();

   return true;
   }


void
printMatrix(complex<double> *M, int64_t N, const char *name)
   {
   printf("%s = [\n", name);
   for (int64_t i=0;i < N;i++)
      {
      printf("      [ (%lf,%lf)", M[i*N].real, M[i*N].imag);
      for (int64_t j=1;j < N;j++)
          printf(", (%lf,%lf)", M[i * N + j].real, M[i * N + j].imag);
      printf(" ],\n");
      }
   printf("    ]\n\n");
   }

int
main(int argc, char *argv[])
   {
   printf("Step 1: initialize JIT\n");
   bool initialized = initializeJit();
   if (!initialized)
      {
      fprintf(stderr, "FAIL: could not initialize JIT\n");
      exit(-1);
      }

   printf("Step 2: define matrices\n");
   const int64_t N=4;
   complex<double> A[N*N];
   complex<double> B[N*N];
   complex<double> C[N*N];
   complex<double> D[N*N];
   for (int64_t i=0;i < N;i++)
      {
      for (int64_t j=0;j < N;j++)
         {
         A[i*N+j] = complex<double>(1.0,0.0);
         B[i*N+j] = complex<double>((double)i,(double)j);
         C[i*N+j] = complex<double>(0.0,0.0);
         D[i*N+j] = complex<double>(0.0,0.0);
         }
      }
   //printMatrix(A, N, "A");
   //printMatrix(B, N, "B");

   printf("Step 3: define type dictionaries\n");
   ComplexTypeDictionary types("ComplexMatMultTypes");

   printf("Step 4: construct MatMult method builder\n");
   ComplexMatMult method(&types);

   std::cerr.setf(std::ios_base::skipws);
   ComplexPrettyPrinter printer(&method, std::cout, std::string("    "));
   method.setLogger(&printer);
   method.config()->setReportMemory()
                  ->setTraceBuildIL()
                  ->setTraceReducer()
                  ->setTraceCodeGenerator();

   bool success = constructFunctionBuilder(&method);
   if (!success)
      {
      fprintf(stderr,"FAIL: construction error\n");
      exit(-2);
      }
   fprintf(stderr, "Builder successfully constructed!\n");

   printf("Print function builder before reduction to jbil_ll dialect\n");
   printer.print();

   printf("Step 5: reduce method builder to jbil_ll dialect\n");
   ComplexReducer reducer(&method, DIALECT(jbil_ll));
   reducer.transform();

   printf("Print function builder after reduction to jbil_ll dialect\n");
   printer.print();

   printf("Step 6: inline AppendBuilders\n");
   AppendBuilderInliner inliner(&method);
   inliner.transform();

   printf("After inlining orphan AppendBuilder\n");
   printer.print();

   printf("Step 7: compile MatMult jbil\n");
   void *entry = NULL;
   int32_t rc = compileFunctionBuilder(&method, &entry);
   if (rc != 0)
      {
      fprintf(stderr,"FAIL: compilation failed %d\n", rc);
      exit(rc);
      }

   printf("Step 8: invoke MatMult compiled code\n");
   ComplexMatMultFunctionType *test = (ComplexMatMultFunctionType *)entry;
   test(C, A, B, N);

   printMatrix(A, N, "A");
   printMatrix(B, N, "B");
   printMatrix(C, N, "C");

   printf ("Step 9: shutdown JIT\n");
   shutdownJit();

   printf("PASS\n");
   }
