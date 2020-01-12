/********************************************************************************
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

#include "PrettyPrinter.hpp"
#include "Builder.hpp"
#include "Case.hpp"
#include "FunctionBuilder.hpp"
#include "Operation.hpp"
#include "Type.hpp"
#include "Value.hpp"

using namespace OMR::JitBuilder;

void
OMR::JitBuilder::PrettyPrinter::visitFunctionBuilderPreOps(FunctionBuilder * fb)
   {
   PrettyPrinter &p = *this;

   p << "[ TypeDictionary " << (fb->types()->id()) << " " << fb->types()->name() << p.endl();
   p.indentIn();
   for (TypeIterator typeIt = fb->types()->TypesBegin();typeIt != fb->types()->TypesEnd();typeIt++)
      {
      Type *type = *typeIt;
      p.indent() << "[ type " << type << " " << type->name()  << " ]" << p.endl();
      }
   p.indentOut();
   p.indent() << "]" << p.endl();

   p << "[ FunctionBuilder MB" << fb->id() << " \"" << fb->name() << "\"" << p.endl();
   p.indentIn();
   p.indent() << "[ types " << fb->types() << " ]" << p.endl();
   p.indent() << "[ origin " << fb->fileName() + "::" + fb->lineNumber() << " ]" << p.endl();
   p.indent() << "[ returnType " << fb->getReturnType() << "]" << p.endl();
   for (ParameterSymbolIterator paramIt = fb->ParametersBegin();paramIt != fb->ParametersEnd(); paramIt++)
      {
      const ParameterSymbol &parameter = *paramIt;
      p.indent() << "[ " << &parameter << " ]" << p.endl();
      }
   for (SymbolIterator localIt = fb->LocalsBegin();localIt != fb->LocalsEnd();localIt++)
      {
      const Symbol &local = *localIt;
      p.indent() << "[ " << &local << " ]" << p.endl();
      }
   p.indent() << "[ operations" << p.endl();
   p.indentIn();
   }

void
OMR::JitBuilder::PrettyPrinter::visitFunctionBuilderPostOps(FunctionBuilder * fb)
   {
   PrettyPrinter &p = *this;

   p.indentOut();
   p.indent() << "]" << p.endl();
   //p.indentOut();
   }

void
OMR::JitBuilder::PrettyPrinter::visitBuilderPreOps(Builder * b)
   {
   // empty (Label) builders aren't printed independently
   if (b->numOperations() > 0)
      {
      PrettyPrinter &p = *this;

      p.indent() << "[ Builder " << b << p.endl();;
      p.indentIn();

      p.indent() << "[ parent " << b->parent() << " ]" << p.endl();

      if (b->numChildren() > 0)
         {
         p.indent() << "[ children" << p.endl();
         p.indentIn();
         for (BuilderIterator bIt = b->ChildrenBegin(); bIt != b->ChildrenEnd(); bIt++)
            {
            Builder *child = *bIt;
            p.indent() << "[ " << child << " ]" << p.endl();
            }
         p.indentOut();
         p.indent() << "]" << p.endl();
         }

      if (b->isBound())
         p.indent() << "[ bound " << b->boundToOperation() << " ]" << p.endl();
      //else
      //   p.indent() << "[ unbound ]" << p.endl();

      if (b->isTarget())
         p.indent() << "[ isTarget ]" << p.endl();
      //else
      //   p.indent() << "[ notTarget ]" << p.endl();

      p.indent() << "[ operations" << p.endl();
      p.indentIn();
      }
   }

void
OMR::JitBuilder::PrettyPrinter::visitBuilderPostOps(Builder * b)
   {
   if (b->numOperations() > 0)
      {
      PrettyPrinter &p = *this;

      p.indentOut();
      p.indent() << "]" << p.endl();
      p.indentOut();
      p.indent() << "]" << p.endl();
      }
   }

void
OMR::JitBuilder::PrettyPrinter::printOperationPrefix(Operation * op)
   {
   PrettyPrinter &p = *this;
   p.indent() << "op" << op->id() << " ( " << op->parent() << " ): ";
   }

void
OMR::JitBuilder::PrettyPrinter::visitOperation(Operation * op)
   {
   PrettyPrinter &p = *this;
   printOperationPrefix(op);
   switch (op->action())
      {
      case aNone :
         break;

      case aConstInt8 :
         p << op->result() << " = ConstInt8 " << op->getLiteralByte() << p.endl();
         break;

      case aConstInt16 :
         p << op->result() << " = ConstInt16 " << op->getLiteralShort() << p.endl();
         break;

      case aConstInt32 :
         p << op->result() << " = ConstInt32 " << op->getLiteralInteger() << p.endl();
         break;

      case aConstInt64 :
         p << op->result() << " = ConstInt64 " << op->getLiteralLong() << p.endl();
         break;

      case aConstFloat :
         p << op->result() << " = ConstFloat " << op->getLiteralFloat() << p.endl();
         break;

      case aConstDouble :
         p << op->result() << " = ConstDouble " << op->getLiteralDouble() << p.endl();
         break;

      case aCoercePointer :
         p << op->result() << " = CoercePointer " << op->type() << " " << op->operand() << p.endl();
         break;

      case aAdd :
         p << op->result() << " = Add " << op->operand(0) << " " << op->operand(1) << p.endl();
         break;

      case aSub :
         p << op->result() << " = Sub " << op->operand(0) << " " << op->operand(1) << p.endl();
         break;

      case aMul :
         p << op->result() << " = Mul " << op->operand(0) << " " << op->operand(1) << p.endl();
         break;

      case aLoad :
         p << op->result() << " = Load " << "\"" << op->getLiteralString() << "\"" << p.endl();
         break;

      case aLoadAt :
         p << op->result() << " = LoadAt " << op->type() << " " << op->operand() << p.endl();
         break;

      case aStore :
         p << "Store \"" << op->getLiteralString() << "\" " << op->operand() << p.endl();
         break;

      case aStoreAt :
         p << "StoreAt " << op->operand(0) << " " << op->operand(1) << p.endl();
         break;

      case aIndexAt :
         p << op->result() << " = IndexAt " << op->type() << " " << op->operand(0) << " " << op->operand(1) << p.endl();
         break;

      case aAppendBuilder :
         {
         Builder * b = op->builder();
         if (b->numOperations() == 0)
            p << "AppendBuilder " << b << " (Label)" << p.endl();
         else
            {
            p << "AppendBuilder " << b << p.endl();
            if (_visitAppendedBuilders)
               {
               p.indentIn();
               start(b);
               p.indentOut();
               }
            }
         }
         break;

      case aReturn :
         p << "Return";
         if (op->numOperands() > 0)
            {
            for (ValueIterator vIt = op->OperandsBegin(); vIt != op->OperandsEnd(); vIt++)
               {
               Value * v = *vIt;
               p << " " << v;
               }
            }
         else
            {
            p << " nil";
            }
         p << p.endl();
         break;

      case aIfCmpGreaterThan :
         p << "IfCmpGreaterThan " << op->operand(0) << " " << op->operand(1);
         p << " then " << op->builder() << p.endl();;
         break;

      case aIfCmpLessThan :
         p << "IfCmpLessThan " << op->operand(0) << " " << op->operand(1);
         p << " then " << op->builder() << p.endl();
         break;

      case aIfCmpGreaterOrEqual :
         p << "IfCmpGreaterOrEqual " << op->operand(0) << " " << op->operand(1);
         p << " then " << op->builder() << p.endl();;
         break;

      case aIfCmpLessOrEqual :
         p << "IfCmpLessOrEqual " << op->operand(0) << " " << op->operand(1);
         p << " then " << op->builder() << p.endl();
         break;

      case aIfThenElse :
         p << "IfThenElse " << op->operand() << " then " << op->builder() << " else ";
         p << " else ";
         if (op->numBuilders() == 2)
            p << op->builder(1);
         else
            p << "nil";
         p << p.endl();
         break;

      case aSwitch :
         p << "Switch " << op->operand(0) << p.endl();
         p.indentIn();
         for (CaseIterator cIt = op->CasesBegin(); cIt != op->CasesEnd(); cIt++)
            {
            Case * c = *cIt;
            p.indent() << c << p.endl();
            }
         p.indentOut();
         break;

      case aForLoop :
         if (op->getLiteralBool())
            p << "ForLoopUp \"";
         else
            p << "ForLoopDn \"";
         p << op->getLiteralString() << "\" : " << op->operand(0) << " to " << op->operand(1) << " by " << op->operand(2);
         p << " body " << op->builder(0);
         if (op->builder(1) != NULL)
            p << " continue " << op->builder(1);
         if (op->builder(2) != NULL)
            p << " break " << op->builder(2);
         p << p.endl();
         break;

      default :
         break;
      }
   }

void
OMR::JitBuilder::PrettyPrinter::visitEnd()
   {
   PrettyPrinter & p = *this;
   p.indentOut();
   p.indent() << "]" << p.endl();
   }
