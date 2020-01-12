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

#include "ComplexPrettyPrinter.hpp"
#include "Operation.hpp"
#include "Value.hpp"

void
ComplexPrettyPrinter::visitOperation(Operation * op)
   {
   ComplexPrettyPrinter &p = *this;
   switch (op->action())
      {
      case aConstComplex :
         printOperationPrefix(op);
         p << op->result() << " = ConstComplex ";
         p << op->getLiteralComplex();
         p << p.endl();
         break;

      case aConjugate :
         printOperationPrefix(op);
         p << op->result() << " = Conjugate " << op->operand(0) << " " << op->operand(1) << p.endl();
         break;

      case aMagnitude :
         printOperationPrefix(op);
         p << op->result() << " = Magnitude " << op->operand() << p.endl();
         break;

      default :
         // otherwise delegate to the base PrettyPrinter
         this->PrettyPrinter::visitOperation(op);
         break;

      }
   }
