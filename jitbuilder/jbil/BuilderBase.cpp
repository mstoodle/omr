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

#include <string>
#include <vector>
#include "Action.hpp"
#include "Operation.hpp"
#include "Builder.hpp"
#include "FunctionBuilder.hpp"
#include "Location.hpp"
#include "PrettyPrinter.hpp"
#include "Value.hpp"
#include "Transformer.hpp"
#include "TypeDictionary.hpp"

using namespace OMR::JitBuilder;

int64_t BuilderBase::globalIndex = 0;

BuilderBase::BuilderBase(Builder * parent, FunctionBuilder * fb)
   : Object(fb)
   , _id(globalIndex++)
   , _name("")
   , _parent(parent)
   , _successor(0)
   , _currentLocation(parent->location())
   , _boundToOperation(NULL)
   , _isTarget(false)
   , _isBound(false)
   , _boundness(May)
   , NoType(TypeDictionary::NoType)
   , Int8(TypeDictionary::Int8)
   , Int16(TypeDictionary::Int16)
   , Int32(TypeDictionary::Int32)
   , Int64(TypeDictionary::Int64)
   , Float(TypeDictionary::Float)
   , Double(TypeDictionary::Double)
   , Address(TypeDictionary::Address)
   {
   if (parent != this) // FunctionBuilders have parent==this, so don't add itself as child
      parent->addChild(self());
   }

BuilderBase::BuilderBase(Builder * parent)
   : Object(parent->fb())
   , _id(globalIndex++)
   , _name("")
   , _parent(parent)
   , _successor(0)
   , _currentLocation(NULL)
   , _boundToOperation(NULL)
   , _isTarget(false)
   , _isBound(false)
   , _boundness(May)
   , NoType(TypeDictionary::NoType)
   , Int8(TypeDictionary::Int8)
   , Int16(TypeDictionary::Int16)
   , Int32(TypeDictionary::Int32)
   , Int64(TypeDictionary::Int64)
   , Float(TypeDictionary::Float)
   , Double(TypeDictionary::Double)
   , Address(TypeDictionary::Address)
   {
   if (parent != this) // FunctionBuilders have parent==this, so don't add itself as child
      parent->addChild(self());
   }

Builder *
BuilderBase::self()
   {
   return static_cast<Builder *>(this);
   }

TypeDictionary *
BuilderBase::types() const
   {
   return _fb->types();
   }

void
BuilderBase::creationError(Action a, std::string msg)
   {
   std::cerr << "Error creating operation " << actionName[a] << "\n";
   std::cerr << "\t(builder b" << _id << ")\n";
   std::cerr << "\t" << msg << "\n";
   assert(0);
   }

void
BuilderBase::creationError(Action a, std::string sName, std::string s)
   {
   std::cerr << "Unknown name creating operation " << actionName[a] << "\n";
   std::cerr << "\t(builder b" << _id << ")\n";
   std::cerr << "\t" << sName << " : " << s << "\n";
   assert(0);
   }

void
BuilderBase::creationError(Action a, std::string vName, Value * v)
   {
   std::cerr << "Incorrect operand type creatign operation " << actionName[a] << "\n";
   std::cerr << "\t(builder b" << _id << ")\n";
   std::cerr << "\t" << vName << " : v" << v->id() << " has type " << v->type()->name() << "\n";
   assert(0);
   }

void
BuilderBase::creationError(Action a, std::string tName, Type * t, std::string vName, Value * v)
   {
   std::cerr << "Incorrect operand types creating operation " << actionName[a] << "\n";
   std::cerr << "\t(builder b" << _id << ")\n";
   std::cerr << "\t" << tName << " : " << t->name() << "\n";
   std::cerr << "\t" << vName << " : v" << v->id() << " has type " << v->type()->name() << "\n";
   assert(0);
   }

void
BuilderBase::creationError(Action a, std::string oneName, Value * one,
                       std::string twoName, Value * two, std::string threeName, Value * three)
   {
   std::cerr << "Incorrect operand types creating operation " << actionName[a] << "\n";
   std::cerr << "\t(builder b" << _id << ")\n";
   std::cerr << "\t" << oneName   << " : v" << one->id()   << " has type " << one->type()->name()   << "\n";
   std::cerr << "\t" << twoName   << " : v" << two->id()   << " has type " << two->type()->name()   << "\n";
   std::cerr << "\t" << threeName << " : v" << three->id() << " has type " << three->type()->name() << "\n";
   assert(0);
   }

void
BuilderBase::creationError(Action a, std::string lName, Value * left, std::string rName, Value * right)
   {
   std::cerr << "Incorrect operand types creating operation " << actionName[a] << "\n";
   std::cerr << "\t(builder b" << _id << ")\n";
   std::cerr << "\t" << lName << " : v" << left->id()  << " has type " << left->type()->name()  << "\n";
   std::cerr << "\t" << rName << " : v" << right->id() << " has type " << right->type()->name() << "\n";
   assert(0);
   }

void
BuilderBase::creationError(Action a, std::string tName, Type * t, std::string firstName, Value * first, std::string secondName, Value * second)
   {
   std::cerr << "Incorrect operand types creating operation " << actionName[a] << "\n";
   std::cerr << "\t(builder b" << _id << ")\n";
   std::cerr << "\t" << tName      << " : " << t->name() << "\n";
   std::cerr << "\t" << firstName  << " : v" << first->id()  << " has type " << first->type()->name()  << "\n";
   std::cerr << "\t" << secondName << " : v" << second->id() << " has type " << second->type()->name() << "\n";
   assert(0);
   }

void
BuilderBase::setParent(Builder *parent)
   {
   _parent = parent;
   _parent->addChild(self());
   }

void
BuilderBase::addChild(Builder *child)
   {
   // shouldn't ever be duplicates, but let's make absolutely sure (assert instead?)
   for (int32_t c=0;c < _children.size();c++)
      if (_children[c] == child)
         return;
   _children.push_back(child);
   }

Builder *
BuilderBase::setBoundness(MustMayCant v)
   {
   assert(v != Must || _isBound == true);
   assert(v != Cant || _isBound == false);
   _boundness = v;
   return self();
   }

void
BuilderBase::checkBoundness(bool v) const
   {
   if (_boundness == May)
         return;

   if (v)
      assert(_boundness == Must);
      else
      assert(_boundness == Cant);
   }

Builder *
BuilderBase::setBound(bool v, Operation * boundToOp)
   {
   checkBoundness(v);
   _isBound = v;
   _boundToOperation = boundToOp;
   return self();
   }

Builder *
BuilderBase::setBound(Operation * boundToOp)
   {
   checkBoundness(true);
   _isBound = true;
   _boundToOperation = boundToOp;
   return self();
   }

Builder *
BuilderBase::setTarget(bool v)
   {
   _isTarget = v;
   return self();
   }

Value *
BuilderBase::ConstInt32 (int32_t v)
   {
   Value * result = Value::create(self(), Int32);
   add(OMR::JitBuilder::ConstInt32::create(self(), result, v));
   return result;
   }

Value *
BuilderBase::ConstInt64 (int64_t v)
   {
   Value * result = Value::create(self(), Int64);
   add(OMR::JitBuilder::ConstInt64::create(self(), result, v));
   return result;
   }

Value *
BuilderBase::ConstDouble (double v)
   {
   Value * result = Value::create(self(), Double);
   add(OMR::JitBuilder::ConstDouble::create(self(), result, v));
   return result;
   }

Value *
BuilderBase::CoercePointer(Type * t, Value * v)
   {
   std::cerr << "v->type()->isPointer() " << (int)(v->type()->isPointer()) << "\n";
   std::cerr << "t->isPointer() " << (int)(t->isPointer()) << "\n";
   if (!v->type()->isPointer() || !t->isPointer())
      creationError(aCoercePointer, "type", t, "value", v);

   // should really verify v has a pointer type
   Value * result = Value::create(self(), t);
   add(OMR::JitBuilder::CoercePointer::create(self(), result, t, v));
   return result;
   }

Value *
BuilderBase::Add (Value * left, Value * right)
   {
   Type *returnType = types()->producedType(aAdd, left, right);
   if (!returnType)
      creationError(aAdd, "left", left, "right", right);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::Add::create(self(), result, left, right));
   return result;
   }

Value *
BuilderBase::Sub (Value * left, Value * right)
   {
   Type *returnType = types()->producedType(aSub, left, right);
   if (!returnType)
      creationError(aSub, "left", left, "right", right);

   Value * result = Value::create(self(), returnType);

   add(OMR::JitBuilder::Sub::create(self(), result, left, right));
   return result;
   }

Value *
BuilderBase::Mul (Value * left, Value * right)
   {
   Type *returnType = types()->producedType(aMul, left, right);
   if (!returnType)
      creationError(aMul, "left", left, "right", right);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::Mul::create(self(), result, left, right));
   return result;
   }

Value *
BuilderBase::IndexAt (Type * type, Value * base, Value * index)
   {
   Type *returnType = types()->producedType(aIndexAt, base, index);
   if (!returnType)
      creationError(aIndexAt, "type", type, "base", base, "index", index);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::IndexAt::create(self(), result, type, base, index));
   return result;
   }

Value *
BuilderBase::Load (std::string name)
   {
   Type *returnType = _fb->getLocalType(name);
   if (!returnType)
      creationError(aLoad, "localName", name);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::Load::create(self(), result, name));
   return result;
   }

Value *
BuilderBase::LoadAt (Type * type, Value * address)
   {
   Type *returnType = types()->producedType(aLoadAt, address);
   if (!returnType)
      creationError(aLoadAt, "type", type, "address", address);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::LoadAt::create(self(), result, type, address));
   return result;
   }

void
BuilderBase::Store (std::string name, Value * value)
   {
   if (fb()->getLocalType(name) == NULL)
      fb()->DefineLocal(name, value->type());

   add(OMR::JitBuilder::Store::create(self(), name, value));
   }

void
BuilderBase::StoreAt (Value * address, Value * value)
   {
   Type *returnType = types()->producedType(aStoreAt, address, value);
   if (!returnType || returnType != NoType)
      creationError(aStoreAt, "address", address, "value", value);

   add(OMR::JitBuilder::StoreAt::create(self(), address, value));
   }

void
BuilderBase::AppendBuilder(Builder * b)
   {
   Operation * appendBuilderOp = OMR::JitBuilder::AppendBuilder::create(self(), b);
   add(appendBuilderOp);
   b->setBoundness(May)
    ->setBound(appendBuilderOp)
    ->setBoundness(Must);
   }

void
BuilderBase::IfCmpGreaterThan(Builder * gtBuilder, Value * left, Value * right)
   {
   Type *returnType = types()->producedType(aIfCmpGreaterThan, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpGreaterThan, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpGreaterThan::create(self(), gtBuilder, left, right));
   gtBuilder->setTarget()
            ->setBoundness(Cant);
   }

void
BuilderBase::IfCmpLessThan(Builder * ltBuilder, Value * left, Value * right)
   {
   Type *returnType = types()->producedType(aIfCmpLessThan, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpLessThan, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpLessThan::create(self(), ltBuilder, left, right));
   ltBuilder->setTarget()
            ->setBoundness(Cant);
   }

void
BuilderBase::IfCmpGreaterOrEqual(Builder * goeBuilder, Value * left, Value * right)
   {
   Type *returnType = types()->producedType(aIfCmpGreaterOrEqual, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpGreaterOrEqual, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpGreaterOrEqual::create(self(), goeBuilder, left, right));
   goeBuilder->setTarget()
             ->setBoundness(Cant);
   }

void
BuilderBase::IfCmpLessOrEqual(Builder * loeBuilder, Value * left, Value * right)
   {
   Type *returnType = types()->producedType(aIfCmpLessOrEqual, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpLessOrEqual, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpLessOrEqual::create(self(), loeBuilder, left, right));
   loeBuilder->setTarget()
             ->setBoundness(Cant);
   }

void
BuilderBase::IfThenElse(Builder * thenB, Builder * elseB, Value * cond)
   {
   if (thenB->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because thenB builder cannot be bound");
   if (elseB->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because elseB builder cannot be bound");

   add(OMR::JitBuilder::IfThenElse::create(self(), thenB, elseB, cond));
   }

void
BuilderBase::ForLoopUp(std::string loopVar, Builder * body, Value * initial, Value * end, Value * bump)
   {
   Type *returnType = types()->producedType(aForLoop, initial, end, bump);
   if (!returnType || returnType != NoType)
      creationError(aForLoop, "initial", initial, "end", end, "bump", bump);
   if (body->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because body builder cannot be bound");

   Operation * forLoopOp = OMR::JitBuilder::ForLoop::create(self(), true, loopVar, body, initial, end, bump);
   add(forLoopOp);

   if (fb()->getLocalType(loopVar) == NULL)
      fb()->DefineLocal(loopVar, initial->type());
   }

void
BuilderBase::ForLoop(bool countsUp, std::string loopVar, Builder * loopBody, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump)
   {
   Type *returnType = types()->producedType(aForLoop, initial, end, bump);
   if (!returnType || returnType != NoType)
      creationError(aForLoop, "initial", initial, "end", end, "bump", bump);
   if (loopBody->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because loopBody builder cannot be bound");
   if (loopContinue->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because loopContinue builder cannot be bound");
   if (loopBreak->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because loopBreak builder cannot be bound");

   Operation * forLoopOp = (OMR::JitBuilder::ForLoop::create(self(), countsUp, loopVar, loopBody, loopContinue, loopBreak, initial, end, bump));
   add(forLoopOp);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);

   if (fb()->getLocalType(loopVar) == NULL)
      fb()->DefineLocal(loopVar, initial->type());
   }

void
BuilderBase::Switch(Value * selector, Builder * defaultBuilder, bool generateBoundsCheck, int numCases, Case ** cases)
   {
   Type *returnType = types()->producedType(aSwitch, selector);
   if (!returnType || returnType != NoType)
      creationError(aSwitch, "selector", selector);
#if 0
   // need to update operands
   add(OMR::JitBuilder::Switch::create(self(), defaultBuilder, selector, generateBoundsCheck, numCases, cases));
#endif
   }

void
BuilderBase::Return()
   {
   add(OMR::JitBuilder::Return::create(self()));
   if (boundness() == Must)
      creationError(aReturn, "Operation invalid because target builder is bound");
   setBoundness(Cant);
   }

Location *
BuilderBase::SourceLocation()
   {
   Location *loc = OMR::JitBuilder::Location::create(_fb);
   _fb->addLocation(loc);
   setLocation(loc);
   return loc;
   }

Location *
BuilderBase::SourceLocation(std::string lineNumber)
   {
   Location *loc = OMR::JitBuilder::Location::create(_fb, lineNumber);
   _fb->addLocation(loc);
   setLocation(loc);
   return loc;
   }

Location *
BuilderBase::SourceLocation(std::string lineNumber, int32_t bcIndex)
   {
   Location *loc = OMR::JitBuilder::Location::create(_fb, lineNumber, bcIndex);
   _fb->addLocation(loc);
   setLocation(loc);
   return loc;
   }

Builder *
BuilderBase::add(Operation * op)
   {
   _fb->registerObject(op);
   op->setLocation(_currentLocation);
   PrettyPrinter *log = _fb->logger();
   if (_fb->config()->traceBuildIL() && log)
      {
      log->indent() << self() << " : create ";
      log->print(op);
      }

   _operations.push_back(op);
   return self();
   }
