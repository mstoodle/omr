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

#ifndef OPERATION_INCL
#define OPERATION_INCL

#include <stdint.h>
#include <string>
#include <vector>
#include "Action.hpp"
#include "complex.hpp"
#include "Iterator.hpp"
#include "OperationBase.hpp"

namespace OMR
{

namespace JitBuilder
{

class Builder;
class Case;
class Location;
class Type;
class TypeDictionary;
class TypeGraph;
class Value;


// Operation defines an interface to all kinds of operations, it cannot itself be instantiated
// Operation classes corresponding to specific Actions (which can be instantiated) follow

class Operation : public OperationBase
   {
   public:
   virtual size_t size() const { return sizeof(Operation); }

   // define any new public API here
   virtual complex<double> getLiteralComplex(int i=0) const { return complex<double>(0,0); }

protected:
   Operation(Action a, Builder * parent);

   // define any new constructors here

   // define any new protected API here

   };

class OneResultOperation : public Operation
   {
   public:
   virtual size_t size() const                { return sizeof(OneResultOperation); }
   virtual ValueIterator ResultsBegin() const { return ValueIterator(_result); }
   virtual ValueIterator ResultsEnd() const   { return ValueIterator(); }
   virtual int32_t numResults() const         { return 1; }
   virtual Value * result(int i=0) const
      {
      if (i == 0) return _result;
      return NULL;
      }

   protected:
   OneResultOperation(Action a, Builder * parent, Value * result)
      : Operation(a, parent)
      , _result(result)
      { }

   Value * _result;
   };

class ConstOperation : public OneResultOperation
   {
   protected:
   ConstOperation(Action a, Builder * parent, Value * result)
      : OneResultOperation(a, parent, result)
      { }

   };

class NoResultOneValueOperation : public Operation
   {
   public:
   virtual size_t size() const         { return sizeof(NoResultOneValueOperation); }
   virtual int32_t numOperands() const { return 1; }
   virtual Value * operand(int i=0) const
      {
      if (i == 0) return _value;
      return NULL;
      }

   virtual ValueIterator OperandsBegin() const { return ValueIterator(_value); }
   virtual ValueIterator OperandsEnd() const   { return ValueIterator(); }

   protected:
   NoResultOneValueOperation(Action a, Builder * parent, Value * value)
      : Operation(a, parent)
      , _value(value)
      { }

   Value * _value;
   };

class OneValueOperation : public OneResultOperation
   {
   public:
   virtual size_t size() const         { return sizeof(OneValueOperation); }
   virtual int32_t numOperands() const { return 1; }
   virtual Value * operand(int i=0) const
      {
      if (i == 0) return _value;
      return NULL;
      }

   virtual ValueIterator OperandsBegin() const { return ValueIterator(_value); }
   virtual ValueIterator OperandsEnd() const   { return ValueIterator(); }

   protected:
   OneValueOperation(Action a, Builder * parent, Value * result, Value * value)
      : OneResultOperation(a, parent, result)
      , _value(value)
      { }

   Value * _value;
   };

class OneTypeOneValueOperation : public OneValueOperation
   {
   public:
   virtual size_t size() const         { return sizeof(OneTypeOneValueOperation); }
   virtual int32_t numTypes() const { return 1; }
   virtual Type * type(int i=0) const
      {
      if (i == 0) return _type;
      return NULL;
      }

   virtual TypeIterator TypesBegin() const { return TypeIterator(_type); }
   virtual TypeIterator TypesEnd() const   { return TypeIterator(); }

   protected:
   OneTypeOneValueOperation(Action a, Builder * parent, Value * result, Type * t, Value * v)
      : OneValueOperation(a, parent, result, v)
      , _type(t)
      { }

   Type * _type;
   };

class NoResultBinaryValueOperation : public Operation
   {
   public:
   virtual size_t size() const         { return sizeof(NoResultBinaryValueOperation); }
   virtual int32_t numOperands() const { return 2; }
   virtual Value * operand(int i=0) const
      {
      if (i == 0) return _left;
      if (i == 1) return _right;
      return NULL;
      }
   virtual Value * getLeft() const  { return _left; }
   virtual Value * getRight() const { return _right; }

   virtual ValueIterator OperandsBegin() const { return ValueIterator(_left, _right); }
   virtual ValueIterator OperandsEnd() const   { return ValueIterator(); }

   protected:
   NoResultBinaryValueOperation(Action a, Builder * parent, Value * left, Value * right)
      : Operation(a, parent)
      , _left(left)
      , _right(right)
      { }

   Value * _left;
   Value * _right;
   };

class BinaryValueOperation : public OneResultOperation
   {
   public:
   virtual size_t size() const         { return sizeof(BinaryValueOperation); }
   virtual int32_t numOperands() const { return 2; }
   virtual Value * operand(int i=0) const
      {
      if (i == 0) return _left;
      if (i == 1) return _right;
      return NULL;
      }
   virtual Value * getLeft() const  { return _left; }
   virtual Value * getRight() const { return _right; }

   virtual ValueIterator OperandsBegin() const { return ValueIterator(_left, _right); }
   virtual ValueIterator OperandsEnd() const   { return ValueIterator(); }

   protected:
   BinaryValueOperation(Action a, Builder * parent, Value * result, Value * left, Value * right)
      : OneResultOperation(a, parent, result)
      , _left(left)
      , _right(right)
      { }

   Value * _left;
   Value * _right;
   };

class OneTypeTwoValueOperation : public BinaryValueOperation
   {
   public:
   virtual size_t size() const         { return sizeof(OneTypeTwoValueOperation); }
   virtual int32_t numTypes() const { return 1; }
   virtual Type * type(int i=0) const
      {
      if (i == 0) return _type;
      return NULL;
      }
   virtual TypeIterator TypesBegin() const { return TypeIterator(_type); }
   virtual TypeIterator TypesEnd() const   { return TypeIterator(); }

   virtual int32_t numOperands() const { return 2; }
   virtual Value *operand(int i=0) const
      {
      if (i == 0) return _left;
      if (i == 1) return _right;
      return NULL;
      }
   virtual Value * getAddress() const  { return _left; }
   virtual Value * getValue() const    { return _right; }
   virtual ValueIterator OperandsBegin() const { return ValueIterator(_left, _right); }
   virtual ValueIterator OperandsEnd() const   { return ValueIterator(); }


   protected:
   OneTypeTwoValueOperation(Action a, Builder * parent, Value * result, Type * t, Value * addr, Value * v)
      : BinaryValueOperation(a, parent, result, addr, v)
      , _type(t)
      { }

   Type * _type;
   };

class LocalVariableOperation : public OneResultOperation
   {
   public:
   virtual size_t size() const { return sizeof(LocalVariableOperation); }
   virtual std::string getLiteralString(int i=0) const { return _name; }

   protected:
   LocalVariableOperation(Action a, Builder * parent, Value * result, std::string name)
      : OneResultOperation(a, parent, result)
      , _name(name)
      { }

   std::string _name;
   };

class LocalVariableOneValueOperation : public Operation
   {
   public:
   virtual size_t size() const { return sizeof(LocalVariableOneValueOperation); }
   virtual std::string getLiteralString(int i=0) const { return _name; }
   virtual int32_t numOperands() const                 { return 1; }
   virtual Value * operand(int i=0) const
      {
      if (i == 0) return _value;
      return NULL;
      }
   virtual ValueIterator OperandsBegin() const        { return ValueIterator(_value); }
   virtual ValueIterator OperandsEnd() const          { return ValueIterator(); }

   protected:
   LocalVariableOneValueOperation(Action a, Builder * parent, std::string name, Value * value)
      : Operation(a, parent)
      , _name(name)
      , _value(value)
      { }

   std::string _name;
   Value * _value;
   };

class OneBuilderOperation : public Operation
   {
   public:
   virtual size_t size() const { return sizeof(OneBuilderOperation); }
   virtual int32_t numBuilders() const { return 1; }
   virtual Builder * builder(int i=0) const
      {
      if (i == 0) return _builder;
      return NULL;
      }

   virtual BuilderIterator BuildersBegin() const { return BuilderIterator(_builder); }
   virtual BuilderIterator BuildersEnd() const   { return BuilderIterator(); }

   protected:
   OneBuilderOperation(Action a, Builder * parent, Builder * b)
      : Operation(a, parent)
      , _builder(b)
      { }

   Builder * _builder;
   };

class OneBuilderBinaryValueOperation : public NoResultBinaryValueOperation
   {
   public:
   virtual size_t size() const { return sizeof(OneBuilderBinaryValueOperation); }
   virtual int32_t numBuilders() const { return 1; }
   virtual Builder * builder(int i=0) const
      {
      if (i == 0) return _builder;
      return NULL;
      }
   virtual BuilderIterator BuildersBegin() const { return BuilderIterator(_builder); }
   virtual BuilderIterator BuildersEnd() const   { return BuilderIterator(); }

   protected:
   OneBuilderBinaryValueOperation(Action a, Builder * parent, Builder * b, Value * left, Value * right)
      : NoResultBinaryValueOperation(a, parent, left, right)
      , _builder(b)
      { }

   Builder * _builder;
   };

class OneBuilderOneValueOperation : public NoResultOneValueOperation
   {
   public:
   virtual size_t size() const { return sizeof(OneBuilderOneValueOperation); }
   virtual int32_t numBuilders() const { return 1; }
   virtual Builder * builder(int i=0) const
      {
      if (i == 0) return _builder;
      return NULL;
      }
   virtual BuilderIterator BuildersBegin() const { return BuilderIterator(_builder); }
   virtual BuilderIterator BuildersEnd() const   { return BuilderIterator(); }

   protected:
   OneBuilderOneValueOperation(Action a, Builder * parent, Builder * b, Value * value)
      : NoResultOneValueOperation(a, parent, value)
      , _builder(b)
      { }

   Builder * _builder;
   };



//
// classes for specific operations that can be directly instantiated
// These classes are manipulated primarily by Builder
//

class ConstInt8 : public ConstOperation
   {
   public:
   virtual size_t size() const { return sizeof(ConstInt8); }
   static ConstInt8 * create(Builder * parent, Value * result, int8_t value)
      { return new ConstInt8(parent, result, value); }

   virtual int8_t getLiteralByte(int i=0) const { return _v; }

   protected:
   ConstInt8(Builder * parent, Value * result, int8_t value);

   int8_t _v;
   };

class ConstInt16 : public ConstOperation
   {
   public:
   virtual size_t size() const { return sizeof(ConstInt16); }
   static ConstInt16 * create(Builder * parent, Value * result, int8_t value)
      { return new ConstInt16(parent, result, value); }

   virtual int16_t getLiteralShort(int i=0) const { return _v; }

   protected:
   ConstInt16(Builder * parent, Value * result, int16_t value);

   int16_t _v;
   };

class ConstInt32 : public ConstOperation
   {
   public:
   virtual size_t size() const { return sizeof(ConstInt32); }
   static ConstInt32 * create(Builder * parent, Value * result, int8_t value)
      { return new ConstInt32(parent, result, value); }

   virtual int32_t getLiteralInteger(int i=0) const { return _v; }

   protected:
   ConstInt32(Builder * parent, Value * result, int32_t value);

   int32_t _v;
   };

class ConstInt64 : public ConstOperation
   {
   public:
   virtual size_t size() const { return sizeof(ConstInt64); }
   static ConstInt64 * create(Builder * parent, Value * result, int8_t value)
      { return new ConstInt64(parent, result, value); }

   virtual int64_t getLiteralLong(int i=0) const { return _v; }

   protected:
   ConstInt64(Builder * parent, Value * result, int64_t value);

   int64_t _v;
   };

class ConstFloat : public ConstOperation
   {
   public:
   virtual size_t size() const { return sizeof(ConstFloat); }
   static ConstFloat * create(Builder * parent, Value * result, int8_t value)
      { return new ConstFloat(parent, result, value); }

   virtual float getLiteralFloat(int i=0) const { return _v; }

   protected:
   ConstFloat(Builder * parent, Value * result, float value);

   float _v;
   };

class ConstDouble : public ConstOperation
   {
   public:
   virtual size_t size() const { return sizeof(ConstDouble); }
   static ConstDouble * create(Builder * parent, Value * result, int8_t value)
      { return new ConstDouble(parent, result, value); }

   virtual double getLiteralDouble(int i=0) const { return _v; }

   protected:
   ConstDouble(Builder * parent, Value * result, double value);

   double _v;
   };

class CoercePointer : public OneTypeOneValueOperation
   {
   public:
   static CoercePointer * create(Builder * parent, Value * result, Type * type, Value * value)
      { return new CoercePointer(parent, result, type, value); }

   protected:
   CoercePointer(Builder * parent, Value * result, Type * t, Value * v);
   };

class Add : public BinaryValueOperation
   {
   public:
   static Add * create(Builder * parent, Value * result, Value * left, Value * right)
      { return new Add(parent, result, left, right); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   Add(Builder * parent, Value * result, Value * left, Value * right);
   };

class Sub : public BinaryValueOperation
   {
   public:
   static Sub * create(Builder * parent, Value * result, Value * left, Value * right)
      { return new Sub(parent, result, left, right); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   Sub(Builder * parent, Value * result, Value * left, Value * right);
   };

class Mul : public BinaryValueOperation
   {
   public:
   static Mul * create(Builder * parent, Value * result, Value * left, Value * right)
      { return new Mul(parent, result, left, right); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   Mul(Builder * parent, Value * result, Value * left, Value * right);
   };

class IndexAt : public OneTypeTwoValueOperation
   {
   public:
   static IndexAt * create(Builder * parent, Value * result, Type * pointerType, Value * address, Value * value)
      { return new IndexAt(parent, result, pointerType, address, value); }
   
   protected:
   IndexAt(Builder * parent, Value * result, Type * pointerType, Value * address, Value * value);
   };

class Load : public LocalVariableOperation
   {
   public:
   static Load * create(Builder * parent, Value * result, std::string name)
      { return new Load(parent, result, name); }
   
   protected:
   Load(Builder * parent, Value * result, std::string name);
   };

class LoadAt : public OneTypeOneValueOperation
   {
   public:
   static LoadAt * create(Builder * parent, Value * result, Type * pointerType, Value * address)
      { return new LoadAt(parent, result, pointerType, address); }
   
   protected:
   LoadAt(Builder * parent, Value * result, Type * pointerType, Value * address);
   };

class Store : public LocalVariableOneValueOperation
   {
   public:
   static Store * create(Builder * parent, std::string name, Value * value)
      { return new Store(parent, name, value); }
   
   protected:
   Store(Builder * parent, std::string name, Value * value);
   };

class StoreAt : public NoResultBinaryValueOperation
   {
   public:
   static StoreAt * create(Builder * parent, Value * address, Value * value)
      { return new StoreAt(parent, address, value); }
   
   virtual Value * getAddress() const { return _left; }
   virtual Value * getValue() const   { return _right; }

   protected:
   StoreAt(Builder * parent, Value * address, Value * value);
   };

class AppendBuilder : public OneBuilderOperation
   {
   public:
   static AppendBuilder * create(Builder * parent, Builder * b)
      { return new AppendBuilder(parent, b); }
   
   protected:
   AppendBuilder(Builder * parent, Builder * b);
   };

class ForLoop : public Operation
   {
   public:
   virtual size_t size() const { return sizeof(ForLoop); }
   static ForLoop * create(Builder * parent, bool countsUp, std::string varName,
                           Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
                           Value * initial, Value * end, Value * bump);
   static ForLoop * create(Builder * parent, bool countsUp, std::string varName,
                           Builder * loopBody, Builder * loopBreak,
                           Value * initial, Value * end, Value * bump);
   static ForLoop * create(Builder * parent, bool countsUp, std::string varName,
                           Builder * loopBody, Value * initial, Value * end, Value * bump);

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   virtual int8_t getLiteralBool(int i=0) const        { return _countsUp; }
   virtual bool countsUp() const                       { return _countsUp; }

   virtual std::string getLiteralString(int i=0) const { return _varName; }
   virtual std::string getLoopVar() const              { return _varName; }

   virtual Value * getInitial() const                  { return _initial; }
   virtual Value * getEnd() const                      { return _end; }
   virtual Value * getBump() const                     { return _bump; }

   virtual int32_t numOperands()                       { return 3; }
   virtual Value * operand(int i=0) const
      {
      if (i == 0) return _initial;
      if (i == 1) return _end;
      if (i == 2) return _bump;
      return NULL;
      }
   virtual ValueIterator OperandsBegin() const         { return ValueIterator(_initial, _end, _bump); }
   virtual ValueIterator OperandsEnd() const           { return ValueIterator(); }

   virtual Builder * getBody() const                   { return _loopBody; }
   virtual Builder * getBreak() const                  { return _loopBreak; }
   virtual Builder * getContinue() const               { return _loopContinue; }
   virtual int32_t numBuilders() const                 { return 1 + (_loopBreak ? 1 : 0) + (_loopContinue ? 1 : 0); }

   virtual Builder * builder(int i=0) const
      {
      if (i == 0) return _loopBody;
      if (i == 1) return _loopBreak;
      if (i == 2) return _loopContinue;
      return NULL;
      }

   virtual BuilderIterator BuildersBegin() const
      {
      if (_loopContinue) return BuilderIterator(_loopBody, _loopBreak, _loopContinue);
      if (_loopBreak)    return BuilderIterator(_loopBody, _loopBreak);
      return BuilderIterator(_loopBody);
      }
   virtual BuilderIterator BuildersEnd() const         { return BuilderIterator(); }

   protected:
   ForLoop(Builder * parent, bool countsUp, std::string varName,
           Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
           Value * initial, Value * end, Value * bump);
   ForLoop(Builder * parent, bool countsUp, std::string varName,
           Builder * loopBody, Builder * loopBreak,
           Value * initial, Value * end, Value * bump);
   ForLoop(Builder * parent, bool countsUp, std::string varName,
           Builder * loopBody,
           Value * initial, Value * end, Value * bump);

   bool _countsUp;
   std::string _varName;

   Builder * _loopBody;
   Builder * _loopBreak;
   Builder * _loopContinue;

   Value * _initial;
   Value * _end;
   Value * _bump;
   };

class IfCmpGreaterThan : public OneBuilderBinaryValueOperation
   {
   public:
   static IfCmpGreaterThan * create(Builder * parent, Builder * tgt, Value * left, Value * right)
      { return new IfCmpGreaterThan(parent, tgt, left, right); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   IfCmpGreaterThan(Builder * parent, Builder * tgt, Value * left, Value * right);
   };

class IfCmpLessThan : public OneBuilderBinaryValueOperation
   {
   public:
   static IfCmpLessThan * create(Builder * parent, Builder * tgt, Value * left, Value * right)
      { return new IfCmpLessThan(parent, tgt, left, right); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   IfCmpLessThan(Builder * parent, Builder * tgt, Value * left, Value * right);
   };

class IfCmpGreaterOrEqual : public OneBuilderBinaryValueOperation
   {
   public:
   static IfCmpGreaterOrEqual * create(Builder * parent, Builder * tgt, Value * left, Value * right)
      { return new IfCmpGreaterOrEqual(parent, tgt, left, right); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   IfCmpGreaterOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right);
   };

class IfCmpLessOrEqual : public OneBuilderBinaryValueOperation
   {
   public:
   static IfCmpLessOrEqual * create(Builder * parent, Builder * tgt, Value * left, Value * right)
      { return new IfCmpLessOrEqual(parent, tgt, left, right); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   IfCmpLessOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right);
   };

class IfThenElse : public OneBuilderOneValueOperation
   {
   public:
   virtual size_t size() const { return sizeof(IfThenElse); }
   static IfThenElse * create(Builder * parent, Builder * thenB, Builder * elseB, Value * cond);
   static IfThenElse * create(Builder * parent, Builder * thenB, Value * cond);

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   virtual Builder * getThenBuilder() const { return _builder; }
   virtual Builder * getElseBuilder() const { return _elseBuilder; }

   virtual int32_t numBuilders() const { return _elseBuilder ? 2 : 1; }
   virtual Builder * builder(int i=0) const
      {
      if (i == 0)
         return _builder;
      else if (i == 1 && _elseBuilder)
         return _elseBuilder;
      return NULL;
      }
   virtual BuilderIterator BuildersBegin() const
      {
      if (_elseBuilder)
         return BuilderIterator(_builder, _elseBuilder);
      return BuilderIterator(_builder);
      }

   virtual BuilderIterator BuildersEnd() const   { return BuilderIterator(); }

   protected:
   IfThenElse(Builder * parent, Builder * thenB, Builder * elseB, Value * cond);
   IfThenElse(Builder * parent, Builder * thenB, Value * cond);

   Builder * _elseBuilder;
   };

class Return : public Operation
   {
   public:
   virtual size_t size() const { return sizeof(Return); }
   static Return * create(Builder * parent)
      { return new Return(parent); }
   static Return * create(Builder * parent, Value * v)
      { return new Return(parent, v); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   virtual int32_t numOperands() const { return _value ? 1 : 0; }
   virtual Value * operand(int32_t i=0) const
      {
      if (i == 0 && _value)
         return _value;
      return NULL;
      }

   virtual ValueIterator OperandsBegin() const
      {
      if (_value)
         return ValueIterator(_value);
      else
         return OperandsEnd();
      }
   virtual ValueIterator OperandsEnd() const { return ValueIterator(); }

   protected:
   Return(Builder * parent);
   Return(Builder * parent, Value * v);

   Value * _value;
   };

class Switch : public NoResultOneValueOperation
   {
   public:
   virtual size_t size() const { return sizeof(Switch); }
   static Switch * create(Builder * parent, Value * selector, int numCases, Case ** cases)
      { return new Switch(parent, selector, numCases, cases); }

   virtual Value * getSelector() const     { return _value; }
   virtual int32_t numCases() const        { return _cases.size(); }
   virtual CaseIterator CasesBegin() const { return _cases.begin(); }
   virtual CaseIterator CasesEnd() const   { return _cases.end(); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   Switch(Builder * parent, Value * selector, int numCases, Case ** cases);

   std::vector<Case *> _cases;
   };

//
// Add any new operation classes here
//

class ConstComplex : public ConstOperation
   {
   public:
   static ConstComplex * create(Builder * parent, Value * result, complex<double> value)
      { return new ConstComplex(parent, result, value); }

   virtual complex<double> getLiteralComplex(int i=0) const { return _v; }

   protected:
   ConstComplex(Builder * parent, Value * result, complex<double> value)
      : ConstOperation(aConstComplex, parent, result)
      , _v(value)
      { }

   complex<double> _v;
   };

class Conjugate : public OneValueOperation
   {
   public:
   static Conjugate * create(Builder * parent, Value * result, Value * value)
      { return new Conjugate(parent, result, value); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   Conjugate(Builder * parent, Value * result, Value * value)
      : OneValueOperation(aConjugate, parent, result, value)
      { }
   };

class Magnitude : public OneValueOperation
   {
   public:
   static Magnitude * create(Builder * parent, Value * result, Value * value)
      { return new Magnitude(parent, result, value); }

   static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

   protected:
   Magnitude(Builder * parent, Value * result, Value * value)
      : OneValueOperation(aMagnitude, parent, result, value)
      { }
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(OPERATION_INCL)
