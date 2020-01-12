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

#ifndef BUILDERBASE_INCL
#define BUILDERBASE_INCL

#include <vector>

namespace OMR
{
namespace JitBuilder
{ 

class Operation;
typedef std::vector<Operation *> OperationVector;
typedef OperationVector::const_iterator OperationIterator;

} // namespace JitBuilder
} // namespace OMR


#include <stdint.h>
#include <vector>

#include "Dialect.hpp"
#include "Object.hpp"
#include "TypeDictionary.hpp"


namespace OMR
{

namespace JitBuilder
{

class Builder;
class Case;
class FunctionBuilder;
class Location;
class Operation;
class Type;
class Value;
class Transformer;

enum MustMayCant {
   Must=0,
   May=1,
   Cant=2
};

class BuilderBase : public Object
   {
   friend class Transformer;

public:

   Value * ConstInt32 (int32_t v);
   Value * ConstInt64 (int64_t v);
   Value * ConstDouble (double v);

   Value * CoercePointer(Type * t, Value * v);

   Value * Add (Value * left, Value * right);
   Value * Sub (Value * left, Value * right);
   Value * Mul (Value * left, Value * right);

   Value * IndexAt (Type * type, Value * base, Value * index);
   Value * Load (std::string name);
   Value * LoadAt (Type * type, Value * address);
   void Store (std::string name, Value * value);
   void StoreAt (Value * address, Value * value);

   void AppendBuilder(Builder * b);
   void IfCmpGreaterThan(Builder * gtBuilder, Value * left, Value * right);
   void IfCmpLessThan(Builder * ltBuilder, Value * left, Value * right);
   void IfCmpGreaterOrEqual(Builder * goeBuilder, Value * left, Value * right);
   void IfCmpLessOrEqual(Builder * loeBuilder, Value * left, Value * right);
   void IfThenElse(Builder * thenB, Value * cond);
   void IfThenElse(Builder * thenB, Builder * elseB, Value * cond);
   void ForLoopUp(std::string loopVar, Builder * body, Value * initial, Value * end, Value * bump);
   void ForLoop(bool countsUp, std::string loopVar, Builder * body, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump);
   void Switch(Value * selector, Builder * defaultBuilder, bool generateBoundsCheck, int numCases, Case ** cases);
   void Return();

   Location * SourceLocation();
   Location * SourceLocation(std::string lineNumber);
   Location * SourceLocation(std::string lineNumber, int32_t bcIndex);

   Type * NoType;
   Type * Int8;
   Type * Int16;
   Type * Int32;
   Type * Int64;
   Type * Float;
   Type * Double;
   Type * Address;

   int64_t id() const                          { return _id; }
   std::string name() const                    { return _name; }
   virtual size_t size() const                 { return sizeof(BuilderBase); }

   TypeDictionary * types() const;
   Builder * parent() const                    { return _parent; }

   int32_t numChildren() const                 { return _children.size(); }
   BuilderIterator ChildrenBegin() const       { return BuilderIterator(_children); }
   BuilderIterator ChildrenEnd() const         { return BuilderIterator(); }

   int32_t numOperations() const               { return _operations.size(); }
   OperationVector & operations()              { return _operations; }
   OperationIterator OperationsBegin() const   { return _operations.begin(); }
   OperationIterator OperationsEnd() const     { return _operations.end(); }

   MustMayCant boundness() const               { return _boundness; }
   Builder * setBoundness(MustMayCant v);
   void checkBoundness(bool v) const;

   bool isBound() const                        { return _isBound; }
   Builder * setBound(bool v, Operation * boundToOp=NULL);
   Builder * setBound(Operation * boundToOp);
   Operation * boundToOperation()              { assert(_isBound); return _boundToOperation; }

   bool isTarget() const                       { return _isTarget; }
   Builder * setTarget(bool v=true);

   static int64_t maxIndex()                   { return globalIndex; }


   protected:
   BuilderBase(Builder * parent, FunctionBuilder * fb);
   BuilderBase(Builder * parent);

   Builder *self();

   void creationError(Action a, std::string msg);
   void creationError(Action a, std::string sName, std::string s);
   void creationError(Action a, std::string vName, Value * v);
   void creationError(Action a, std::string tName, Type * t, std::string vName, Value * v);
   void creationError(Action a, std::string lName, Value * left, std::string rName, Value * right);
   void creationError(Action a, std::string oneName, Value * one, std::string twoName, Value * two, std::string threeName, Value * three);
   void creationError(Action a, std::string tName, Type * t, std::string firstName, Value * first, std::string secondName, Value * second);

   void setParent(Builder *parent);
   void addChild(Builder *child);
   Builder * add(Operation * op);
   Location *location() const
      {
      return _currentLocation;
      }
   void setLocation(Location * loc)
      {
      _currentLocation = loc;
      }

   int64_t                    _id;
   std::string                _name;
   Builder                  * _parent;
   std::vector<Builder *>     _children;
   Builder                  * _successor;
   OperationVector            _operations;
   Location                 * _currentLocation;
   Operation                * _boundToOperation;
   bool                       _isTarget;
   bool                       _isBound;
   MustMayCant                _boundness;
   static int64_t             globalIndex;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(BUILDERBASE_INCL)
