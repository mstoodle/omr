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

#ifndef OPERATIONBASE_INCL
#define OPERATIONBASE_INCL

#include <stdint.h>
#include <string>
#include <vector>
#include "Action.hpp"
#include "Dialect.hpp"
#include "Iterator.hpp"
#include "Object.hpp"

namespace OMR
{

namespace JitBuilder
{

class Builder;
class Case;
class Location;
class Operation;
class Type;
class TypeDictionary;
class TypeGraph;
class Value;


class OperationBase : public Object
   {
   friend class Builder;
   friend class BuilderBase;
   friend class Transformer;

   public:
   int64_t id() const                                  { return _index; }
   Action action() const                               { return _action; }
   Builder * parent() const                            { return _parent; }
   Location * location() const                         { return _location; }

   virtual int8_t getLiteralBool(int i=0) const        { assert(0); return 0; }
   virtual int8_t getLiteralByte(int i=0) const        { assert(0); return 0; }
   virtual int16_t getLiteralShort(int i=0) const      { assert(0); return 0; }
   virtual int32_t getLiteralInteger(int i=0) const    { assert(0); return 0; }
   virtual int64_t getLiteralLong(int i=0) const       { assert(0); return 0; }
   virtual float getLiteralFloat(int i=0) const        { assert(0); return 0.0; }
   virtual double getLiteralDouble(int i=0) const      { assert(0); return 0.0; }
   virtual std::string getLiteralString(int i=0) const { assert(0); return ""; }

   virtual ValueIterator OperandsBegin() const         { return ValueIterator(); }
   virtual ValueIterator OperandsEnd() const           { return ValueIterator(); }
   virtual int32_t numOperands() const                 { return 0; }
   virtual Value * operand(int i=0) const              { return NULL; }

   virtual ValueIterator ResultsBegin() const          { return ValueIterator(); }
   virtual ValueIterator ResultsEnd() const            { return ValueIterator(); }
   virtual int32_t numResults() const                  { return 0; }
   virtual Value * result(int i=0) const               { return NULL; }
 
   virtual BuilderIterator BuildersBegin() const       { return BuilderIterator(); }
   virtual BuilderIterator BuildersEnd() const         { return BuilderIterator(); }
   virtual int32_t numBuilders() const                 { return 0; }
   virtual Builder *builder(int i=0) const             { return NULL; }

   virtual CaseIterator CasesBegin() const             { return CaseIterator(); }
   virtual CaseIterator CasesEnd() const               { return CaseIterator(); }
   virtual int32_t numCases() const                    { return 0; }
   virtual Case * getCase(int i=0) const               { return NULL; }

   virtual TypeIterator TypesBegin() const             { return TypeIterator(); }
   virtual TypeIterator TypesEnd() const               { return TypeIterator(); }
   virtual int32_t numTypes() const                    { return 0; }
   virtual Type * type(int i=0) const                  { return NULL; }

protected:
   OperationBase(Action a, Builder * parent);

   Operation * setParent(Builder * newParent);
   Operation * setLocation(Location *location);

   int64_t                  _index;
   Builder                * _parent;
   Action                   _action;
   Location               * _location;

   static int64_t globalIndex;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(OPERATIONBASE_INCL)
