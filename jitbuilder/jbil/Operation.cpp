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

#include <stdint.h>
#include "Builder.hpp"
#include "FunctionBuilder.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "TypeDictionary.hpp"
#include "TypeGraph.hpp"
#include "Value.hpp"


OMR::JitBuilder::Operation::Operation(Action a, Builder * parent)
   : OperationBase(a, parent->fb())
   {
   } 

OMR::JitBuilder::ConstInt8::ConstInt8(Builder * parent, Value * result, int8_t value)
   : ConstOperation(aConstInt8, parent, result)
   , _v(value)
   {
   }

OMR::JitBuilder::ConstInt16::ConstInt16(Builder * parent, Value * result, int16_t value)
   : ConstOperation(aConstInt16, parent, result)
   , _v(value)
   {
   }

OMR::JitBuilder::ConstInt32::ConstInt32(Builder * parent, Value * result, int32_t value)
   : ConstOperation(aConstInt32, parent, result)
   , _v(value)
   {
   }

OMR::JitBuilder::ConstInt64::ConstInt64(Builder * parent, Value * result, int64_t value)
   : ConstOperation(aConstInt64, parent, result)
   , _v(value)
   {
   }

OMR::JitBuilder::ConstFloat::ConstFloat(Builder * parent, Value * result, float value)
   : ConstOperation(aConstFloat, parent, result)
   , _v(value)
   {
   }

OMR::JitBuilder::ConstDouble::ConstDouble(Builder * parent, Value * result, double value)
   : ConstOperation(aConstDouble, parent, result)
   , _v(value)
   {
   }

OMR::JitBuilder::CoercePointer::CoercePointer(Builder * parent, Value * result, Type * t, Value * v)
   : OneTypeOneValueOperation(aCoercePointer, parent, result, t, v)
   {
   assert(t->isPointer() && v->type()->isPointer());
   }

OMR::JitBuilder::Add::Add(Builder * parent, Value * result, Value * left, Value * right)
   : BinaryValueOperation(aAdd, parent, result, left, right)
   {
   }

void
OMR::JitBuilder::Add::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   Type * I8 = types->Int8; graph->registerValidOperation(I8, aAdd, I8, I8);
   graph->registerValidOperation(types->Int16 , aAdd, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->Int32 , aAdd, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->Int64 , aAdd, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->Float , aAdd, types->Float , types->Float );
   graph->registerValidOperation(types->Double, aAdd, types->Double, types->Double);

   graph->registerValidOperation(types->Address, aAdd, types->Address, types->Word);
   }


OMR::JitBuilder::Sub::Sub(Builder * parent, Value * result, Value * left, Value * right)
   : BinaryValueOperation(aSub, parent, result, left, right)
   {
   }

void
OMR::JitBuilder::Sub::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->Int8  , aSub, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->Int16 , aSub, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->Int32 , aSub, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->Int64 , aSub, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->Float , aSub, types->Float , types->Float );
   graph->registerValidOperation(types->Double, aSub, types->Double, types->Double);

   graph->registerValidOperation(types->Address, aSub, types->Address, types->Word);
   }

OMR::JitBuilder::Mul::Mul(Builder * parent, Value * result, Value * left, Value * right)
   : BinaryValueOperation(aMul, parent, result, left, right)
   {
   }

void
OMR::JitBuilder::Mul::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->Int8  , aMul, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->Int16 , aMul, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->Int32 , aMul, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->Int64 , aMul, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->Float , aMul, types->Float , types->Float );
   graph->registerValidOperation(types->Double, aMul, types->Double, types->Double);
   }

OMR::JitBuilder::IndexAt::IndexAt(Builder * parent, Value * result, Type * pointerType, Value * address, Value * value)
   : OneTypeTwoValueOperation(aIndexAt, parent, result, pointerType, address, value)
   {
   }

OMR::JitBuilder::Load::Load(Builder * parent, Value * result, std::string name)
   : LocalVariableOperation(aLoad, parent, result, name)
   {
   }

OMR::JitBuilder::LoadAt::LoadAt(Builder * parent, Value * result, Type * pointerType, Value * address)
   : OneTypeOneValueOperation(aLoadAt, parent, result, pointerType, address)
   {
   }

OMR::JitBuilder::Store::Store(Builder * parent, std::string name, Value * value)
   : LocalVariableOneValueOperation(aStore, parent, name, value)
   {
   }

OMR::JitBuilder::StoreAt::StoreAt(Builder * parent, Value * address, Value * value)
   : NoResultBinaryValueOperation(aStoreAt, parent, address, value)
   {
   }

OMR::JitBuilder::AppendBuilder::AppendBuilder(Builder * parent, Builder * b)
   : OneBuilderOperation(aAppendBuilder, parent, b)
   {
   }

OMR::JitBuilder::ForLoop::ForLoop(Builder * parent, bool countsUp, std::string varName,
                                  Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
                                  Value * initial, Value * end, Value * bump)
   : Operation(aForLoop, parent)
   , _countsUp(countsUp)
   , _varName(varName)
   , _loopBody(loopBody)
   , _loopBreak(loopBreak)
   , _loopContinue(loopContinue)
   , _initial(initial)
   , _end(end)
   , _bump(bump)
   {
   }

OMR::JitBuilder::ForLoop::ForLoop(Builder * parent, bool countsUp, std::string varName,
                                  Builder * loopBody, Builder * loopBreak,
                                  Value * initial, Value * end, Value * bump)
   : Operation(aForLoop, parent)
   , _countsUp(countsUp)
   , _varName(varName)
   , _loopBody(loopBody)
   , _loopBreak(loopBreak)
   , _loopContinue(NULL)
   , _initial(initial)
   , _end(end)
   , _bump(bump)
   {
   }

OMR::JitBuilder::ForLoop::ForLoop(Builder * parent, bool countsUp, std::string varName,
                                  Builder * loopBody,
                                  Value * initial, Value * end, Value * bump)
   : Operation(aForLoop, parent)
   , _countsUp(countsUp)
   , _varName(varName)
   , _loopBody(loopBody)
   , _loopBreak(NULL)
   , _loopContinue(NULL)
   , _initial(initial)
   , _end(end)
   , _bump(bump)
   {
   }

OMR::JitBuilder::ForLoop *
OMR::JitBuilder::ForLoop::create(Builder * parent, bool countsUp, std::string varName,
                           Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
                           Value * initial, Value * end, Value * bump)
   {
   OMR::JitBuilder::ForLoop * forLoopOp = new ForLoop(parent, countsUp, varName,
                                                      loopBody, loopBreak, loopContinue,
                                                      initial, end, bump);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);
   loopBreak->setTarget()
            ->setBoundness(May)
            ->setBound(forLoopOp)
            ->setBoundness(Must);
   loopContinue->setTarget()
               ->setBoundness(May)
               ->setBound(forLoopOp)
               ->setBoundness(Must);
   return forLoopOp;
   }

OMR::JitBuilder::ForLoop *
OMR::JitBuilder::ForLoop::create(Builder * parent, bool countsUp, std::string varName,
                           Builder * loopBody, Builder * loopBreak,
                           Value * initial, Value * end, Value * bump)
   {
   OMR::JitBuilder::ForLoop * forLoopOp = new ForLoop(parent, countsUp, varName, loopBody, loopBreak,
                                                      initial, end, bump);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);
   loopBreak->setTarget()
            ->setBoundness(May)
            ->setBound(forLoopOp)
            ->setBoundness(Must);
   return forLoopOp;
   }

OMR::JitBuilder::ForLoop *
OMR::JitBuilder::ForLoop::create(Builder * parent, bool countsUp, std::string varName,
                           Builder * loopBody, Value * initial, Value * end, Value * bump)
   {
   OMR::JitBuilder::ForLoop * forLoopOp = new ForLoop(parent, countsUp, varName, loopBody, initial, end, bump);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);
   return forLoopOp;
   }

void
OMR::JitBuilder::ForLoop::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aForLoop, types->Int8  , types->Int8  , types->Int8 );
   graph->registerValidOperation(types->NoType, aForLoop, types->Int16 , types->Int16 , types->Int16);
   graph->registerValidOperation(types->NoType, aForLoop, types->Int32 , types->Int32 , types->Int32);
   graph->registerValidOperation(types->NoType, aForLoop, types->Int64 , types->Int64 , types->Int64);
   graph->registerValidOperation(types->NoType, aForLoop, types->Float , types->Float , types->Float);
   graph->registerValidOperation(types->NoType, aForLoop, types->Double, types->Double, types->Double);
   }

OMR::JitBuilder::IfCmpGreaterThan::IfCmpGreaterThan(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OneBuilderBinaryValueOperation(aIfCmpGreaterThan, parent, tgt, left, right)
   {
   }

void
OMR::JitBuilder::IfCmpGreaterThan::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Double, types->Double);
   }

OMR::JitBuilder::IfCmpLessThan::IfCmpLessThan(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OneBuilderBinaryValueOperation(aIfCmpLessThan, parent, tgt, left, right)
   {
   }

void
OMR::JitBuilder::IfCmpLessThan::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Double, types->Double);
   }

OMR::JitBuilder::IfCmpGreaterOrEqual::IfCmpGreaterOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OneBuilderBinaryValueOperation(aIfCmpGreaterOrEqual, parent, tgt, left, right)
   {
   }

void
OMR::JitBuilder::IfCmpGreaterOrEqual::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Double, types->Double);
   }

OMR::JitBuilder::IfCmpLessOrEqual::IfCmpLessOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OneBuilderBinaryValueOperation(aIfCmpLessOrEqual, parent, tgt, left, right)
   {
   }

void
OMR::JitBuilder::IfCmpLessOrEqual::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Double, types->Double);
   }

OMR::JitBuilder::IfThenElse::IfThenElse(Builder * parent, Builder * thenB, Builder * elseB, Value * cond)
   : OneBuilderOneValueOperation(aIfThenElse, parent, thenB, cond)
   , _elseBuilder(elseB)
   {
   }

OMR::JitBuilder::IfThenElse::IfThenElse(Builder * parent, Builder * thenB, Value * cond)
   : OneBuilderOneValueOperation(aIfThenElse, parent, thenB, cond)
   , _elseBuilder(NULL)
   {
   }

OMR::JitBuilder::IfThenElse *
OMR::JitBuilder::IfThenElse::create(Builder * parent, Builder * thenB, Builder * elseB, Value * cond)
   {
   OMR::JitBuilder::IfThenElse * ifThenElseOp = new IfThenElse(parent, thenB, elseB, cond);
   thenB->setTarget()
        ->setBoundness(May)
        ->setBound(ifThenElseOp)
        ->setBoundness(Must);
   elseB->setTarget()
        ->setBoundness(May)
        ->setBound(ifThenElseOp)
        ->setBoundness(Must);
   return ifThenElseOp;
   }

OMR::JitBuilder::IfThenElse *
OMR::JitBuilder::IfThenElse::create(Builder * parent, Builder * thenB, Value * cond)
   {
   OMR::JitBuilder::IfThenElse * ifThenOp = new IfThenElse(parent, thenB, cond);
   thenB->setTarget()
        ->setBoundness(May)
        ->setBound(ifThenOp)
        ->setBoundness(Must);
   return ifThenOp;
   }

void
OMR::JitBuilder::IfThenElse::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int8);
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int16);
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int32);
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int64);
   }

OMR::JitBuilder::Return::Return(Builder * parent)
   : Operation(aReturn, parent), _value(NULL)
   {
   }

OMR::JitBuilder::Return::Return(Builder * parent, Value * v)
   : Operation(aReturn, parent), _value(v)
   {
   }

void
OMR::JitBuilder::Return::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aReturn, types->Int8);
   graph->registerValidOperation(types->NoType, aReturn, types->Int16);
   graph->registerValidOperation(types->NoType, aReturn, types->Int32);
   graph->registerValidOperation(types->NoType, aReturn, types->Int64);
   graph->registerValidOperation(types->NoType, aReturn, types->Float);
   graph->registerValidOperation(types->NoType, aReturn, types->Double);
   }

OMR::JitBuilder::Switch::Switch(Builder * parent, Value * selector, int numCases, Case ** cases)
   : NoResultOneValueOperation(aSwitch, parent, selector)
   {
   for (int c=0;c < numCases;c++)
      _cases[c] = cases[c];
   }

void
OMR::JitBuilder::Switch::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aSwitch, types->Int32);
   }
