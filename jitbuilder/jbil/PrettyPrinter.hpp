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

#ifndef PRETTYPRINTER_INCL
#define PRETTYPRINTER_INCL


#include <iostream>
#include <vector>
#include <deque>
#include "Visitor.hpp"
#include "Builder.hpp"
#include "Case.hpp"
#include "FunctionBuilder.hpp"
#include "Operation.hpp"
#include "Symbol.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "TypeGraph.hpp"
#include "Value.hpp"

namespace OMR
{

namespace JitBuilder
{

class PrettyPrinter : public Visitor
   {
public:
   PrettyPrinter(FunctionBuilder * fb, std::ostream & os, std::string perIndent)
      : Visitor(fb)
      , _os(os)
      , _perIndent(perIndent)
      , _indent(0)
      { }

   void print()               { start(); }
   void print(Builder * b)    { start(b); }
   void print(Operation * op) { start(op); }

   friend PrettyPrinter &operator<<(PrettyPrinter &p, const bool v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const int8_t v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const int16_t v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const int32_t v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const int64_t v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const uint64_t v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const void * v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const float v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const double v)
      {
      p._os << v;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const std::string & s)
      {
      p._os << s;
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter &p, const char *s)
      {
      p._os << s;
      return p;
      }
   friend PrettyPrinter & operator<<(PrettyPrinter &p, const Builder *b)
      {
      p << "B" << b->id() << " \"" << b->name() << "\"";
      return p;
      }
   friend PrettyPrinter &operator<<(PrettyPrinter & p, const Case *c)
      {
      p << "[ case " << c->value() << " : " << c->builder() << " ] " << p.endl();
      return p;
      }
   friend PrettyPrinter & operator<< (PrettyPrinter &p, FunctionBuilder *fb)
      {
      return p << "fb" << fb->id();
      }
   friend PrettyPrinter & operator<< (PrettyPrinter &p, Operation *op)
      {
      return p << "op" << op->id();
      }
   friend PrettyPrinter & operator<< (PrettyPrinter &p, const ParameterSymbol *param)
      {
      return p << "parameter \"" << param->name() << "\" " << param->type() << " " << param->index();
      }
   friend PrettyPrinter & operator<< (PrettyPrinter &p, const Symbol *s)
      {
      return p << "local \"" << s->name() << "\" " << s->type();
      }
   friend PrettyPrinter &operator<<(PrettyPrinter & p, const Type *t)
      {
      return p << "t" << t->id();
      }
   friend PrettyPrinter & operator<<(PrettyPrinter & p, const TypeDictionary *dict)
      {
      return p << "td" << dict->id();
      }
   friend PrettyPrinter & operator<<(PrettyPrinter & p, const TypeGraph *graph)
      {
      return p << "tg" << graph->id();
      }
   friend PrettyPrinter &operator<<(PrettyPrinter & p, const Value *v)
      {
      return p << "v" << v->id();
      }


   std::string endl()
      {
      return std::string("\n");
      }

   PrettyPrinter & indent()
      {
      for (int32_t in=0;in < _indent;in++)
         _os << _perIndent;
      return *this;
      }
   void indentIn()
      {
      _indent++;
      }
   void indentOut()
      {
      _indent--;
      }

   protected:

      virtual void visitFunctionBuilderPreOps(FunctionBuilder * fb);
      virtual void visitFunctionBuilderPostOps(FunctionBuilder * fb);
      virtual void visitBuilderPreOps(Builder * b);
      virtual void visitBuilderPostOps(Builder * b);
      virtual void visitOperation(Operation * op);
      virtual void visitEnd();

      void printOperationPrefix(Operation * op);

      std::ostream & _os;
      std::string _perIndent;
      int32_t _indent;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(PRETTYPRINTER_INCL)
