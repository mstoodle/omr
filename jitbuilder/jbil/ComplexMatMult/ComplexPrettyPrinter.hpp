/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
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

#ifndef COMPLEXPRETTYPRINTER_INCL
#define COMPLEXPRETTYPRINTER_INCL


#include "PrettyPrinter.hpp"
#include "complex.hpp"


using namespace OMR::JitBuilder;

class ComplexPrettyPrinter : public PrettyPrinter
   {
public:
   ComplexPrettyPrinter(FunctionBuilder *fb, std::ostream & os, std::string perIndent)
      : PrettyPrinter(fb, os, perIndent)
      { }

   virtual void visitOperation(Operation *op);

   friend ComplexPrettyPrinter &operator<<(ComplexPrettyPrinter &p, const complex<double> v)
      {
      p << v.real << "+" << v.imag << "i";
      return p;
      }
   };

#endif // defined(COMPLEXPRETTYPRINTER_INCL)
