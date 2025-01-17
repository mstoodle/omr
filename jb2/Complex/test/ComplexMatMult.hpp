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


#ifndef COMPLEXMATMULT_INCL
#define COMPLEXMATMULT_INCL

#include <complex>
#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Func/Func.hpp"
#include "Complex/Complex.hpp"

using namespace OMR::JB2;

typedef void (ComplexMatMultFunctionType)(std::complex<double> *, std::complex<double> *, std::complex<double> *, int64_t);

class ComplexMatMult : public Func::Function
   {
   private:
   Complex::ComplexExtension *_cx;
   Base::BaseExtension *_bx;

   const Type *pComplex;

   Func::ParameterSymbol *_paramC;
   Func::ParameterSymbol *_paramA;
   Func::ParameterSymbol *_paramB;
   Func::ParameterSymbol *_paramN;
   Func::LocalSymbol *_sumVar;

   Value *elementAt(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N);
   void Store2D(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N, Value *value);
   Value *Load2D(LOCATION, Builder *bldr, Value *base, Value *first, Value *second, Value *N);

   public:
   ComplexMatMult(MEM_LOCATION(a), Compiler *compiler, Complex::ComplexExtension *xc);
   virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);
   virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);
   };

#endif // !defined(COMPLEXMATMULT_INCL)
