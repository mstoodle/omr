/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
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
#include "BaseExtension.hpp"
#include "BaseSymbols.hpp"
#include "BaseTypes.hpp"
#include "Builder.hpp"
#include "ControlOperations.hpp"
#include "Function.hpp"
#include "JB1MethodBuilder.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "MemoryOperations.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "TextWriter.hpp"
#include "Value.hpp"

using namespace OMR::JitBuilder;

namespace OMR {
namespace JitBuilder {
namespace Base {

//
// Load
//
Op_Load::Op_Load(LOCATION, Extension *ext, Builder * parent, ActionID aLoad, Value *result, Symbol *symbol)
    : OperationR1S1(PASSLOC, aLoad, ext, parent, result, symbol) {
}

Operation *
Op_Load::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Load(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->symbol());
}

void
Op_Load::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Load(location(), this->parent(), this->_result, this->_symbol);
}


//
// Store
//
Op_Store::Op_Store(LOCATION, Extension *ext, Builder * parent, ActionID aStore, Symbol *symbol, Value *value)
    : OperationR0S1V1(PASSLOC, aStore, ext, parent, symbol, value) {
    assert(value);
}

Operation *
Op_Store::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Store(PASSLOC, this->_ext, b, this->action(), cloner->symbol(), cloner->operand());
}

void
Op_Store::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Store(location(), this->parent(), this->_symbol, this->_value);
}


//
// LoadAt
//
Op_LoadAt::Op_LoadAt(LOCATION, Extension *ext, Builder * parent, ActionID aLoadAt, Value *result, Value *ptrValue)
    : OperationR1V1(PASSLOC, aLoadAt, ext, parent, result, ptrValue) {
}

Operation *
Op_LoadAt::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_LoadAt(PASSLOC, this->_ext, b, this->action(), this->result(), this->operand());
}

void
Op_LoadAt::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->LoadAt(location(), this->parent(), this->_result, this->_value);
}


//
// StoreAt
//
Op_StoreAt::Op_StoreAt(LOCATION, Extension *ext, Builder * parent, ActionID aStoreAt, Value *ptrValue, Value *value)
    : OperationR0V2(PASSLOC, aStoreAt, ext, parent, ptrValue, value) {
}

Operation *
Op_StoreAt::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_StoreAt(PASSLOC, this->_ext, b, this->action(), cloner->operand(0), this->operand(1));
}

void
Op_StoreAt::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->StoreAt(location(), this->parent(), this->_left, this->_right);
}


//
// LoadField
//
Op_LoadField::Op_LoadField(LOCATION, Extension *ext, Builder * parent, ActionID aLoadField, Value *result, const FieldType *fieldType, Value *structValue)
    : OperationR1V1T1(PASSLOC, aLoadField, ext, parent, result, fieldType, structValue) {
}

Operation *
Op_LoadField::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_LoadField(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->type()->refine<FieldType>(), cloner->operand());
}

void
Op_LoadField::jbgen(JB1MethodBuilder *j1mb) const {
    assert(0); // needs to be expanded
}


//
// StoreField
//
Op_StoreField::Op_StoreField(LOCATION, Extension *ext, Builder * parent, ActionID aStoreField, const FieldType *fieldType, Value *structValue, Value *value)
    : OperationR0T1V2(PASSLOC, aStoreField, ext, parent, fieldType, structValue, value) {
}

Operation *
Op_StoreField::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_StoreField(PASSLOC, this->_ext, b, this->action(), cloner->type()->refine<FieldType>(), cloner->operand(0), cloner->operand(1));
}

void
Op_StoreField::jbgen(JB1MethodBuilder *j1mb) const {
    assert(0); // needs to be expanded
}


//
// LoadFieldAt
//
Op_LoadFieldAt::Op_LoadFieldAt(LOCATION, Extension *ext, Builder * parent, ActionID aLoadFieldAt, Value *result, const FieldType *fieldType, Value *pStruct)
    : OperationR1V1T1(PASSLOC, aLoadFieldAt, ext, parent, result, fieldType, pStruct) {
}

Operation *
Op_LoadFieldAt::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_LoadFieldAt(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->type()->refine<FieldType>(), cloner->operand());
}

void
Op_LoadFieldAt::jbgen(JB1MethodBuilder *j1mb) const {
    const FieldType *fType = _type->refine<FieldType>();
    const StructType *sType = fType->owningStruct();
    j1mb->LoadIndirect(location(), this->parent(), this->_result, sType->name(), fType->name(), this->_value);
}


//
// StoreFieldAt
//
Op_StoreFieldAt::Op_StoreFieldAt(LOCATION, Extension *ext, Builder * parent, ActionID aStoreFieldAt, const FieldType *fieldType, Value *pStruct, Value *value)
    : OperationR0T1V2(PASSLOC, aStoreFieldAt, ext, parent, fieldType, pStruct, value) {
}

Operation *
Op_StoreFieldAt::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_StoreFieldAt(PASSLOC, this->_ext, b, this->action(), cloner->type()->refine<FieldType>(), cloner->operand(0), cloner->operand(1));
}

void
Op_StoreFieldAt::jbgen(JB1MethodBuilder *j1mb) const {
    const FieldType *fType = _type->refine<FieldType>();
    const StructType *sType = fType->owningStruct();
    j1mb->StoreIndirect(location(), this->parent(), sType->name(), fType->name(), this->_base, this->_value);
}


//
// CreateLocalArray
//
Operation *
Op_CreateLocalArray::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    const Type *cloneType = cloner->type();
    assert(cloneType->isKind<PointerType>());
    return new Op_CreateLocalArray(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->literal(), cloneType->refine<PointerType>());
}

void
Op_CreateLocalArray::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->CreateLocalArray(location(), this->parent(), this->result(), this->literal(), this->type());
}


//
// CreateLocalStruct
//
Operation *
Op_CreateLocalStruct::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    const Type *cloneType = cloner->type();
    assert(cloneType->isKind<StructType>());
    return new Op_CreateLocalStruct(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloneType->refine<StructType>());
}

void
Op_CreateLocalStruct::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->CreateLocalStruct(location(), this->parent(), this->result(), this->type());
}


//
// IndexAt
//
Op_IndexAt::Op_IndexAt(LOCATION, Extension *ext, Builder * parent, ActionID aIndexAt, Value *result, Value *base, Value *index)
    : OperationR1V2(PASSLOC, aIndexAt, ext, parent, result, base, index) {
}

Operation *
Op_IndexAt::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IndexAt(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0), cloner->operand(1));
}

void
Op_IndexAt::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IndexAt(location(), this->parent(), this->_result, this->_left, this->_right);
}


///////////////////////////////////////////////////////////////////////////////////
#if 0
CoercePointer::CoercePointer(Builder * parent, Value * result, Type * t, Value * v)
   : OperationR1V1T1(aCoercePointer, parent, result, t, v)
   {
   assert(t->isPointer() && v->type()->isPointer());
   }

void
CoercePointer::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->CoercePointer(typeMappers[0]->next(), operandMappers[0]->next()) );
   }

Operation *
CoercePointer::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->type(), cloner->operand());
   }

void
CoercePointer::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->Address, aCoercePointer, types->Address);
   }

Add::Add(Builder * parent, Value * result, Value * left, Value * right)
   : OperationR1V2(aAdd, parent, result, left, right)
   {
   }

void
Add::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->Add(operandMappers[0]->next(), operandMappers[1]->next()) );
   }

Operation *
Add::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->operand(0), cloner->operand(1));
   }

void
Add::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->Int8,    aAdd, types->Int8,    types->Int8 );
   graph->registerValidOperation(types->Int16,   aAdd, types->Int16,   types->Int16);
   graph->registerValidOperation(types->Int32,   aAdd, types->Int32,   types->Int32);
   graph->registerValidOperation(types->Int64,   aAdd, types->Int64,   types->Int64);
   graph->registerValidOperation(types->Float,   aAdd, types->Float,   types->Float);
   graph->registerValidOperation(types->Double,  aAdd, types->Double,  types->Double);
   graph->registerValidOperation(types->Address, aAdd, types->Address, types->Word);
   }


Sub::Sub(Builder * parent, Value * result, Value * left, Value * right)
   : OperationR1V2(aSub, parent, result, left, right)
   {
   }

void
Sub::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->Sub(operandMappers[0]->next(), operandMappers[1]->next()) );
   }

Operation *
Sub::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->operand(0), cloner->operand(1));
   }

void
Sub::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->Int8,    aSub, types->Int8,    types->Int8 );
   graph->registerValidOperation(types->Int16,   aSub, types->Int16,   types->Int16);
   graph->registerValidOperation(types->Int32,   aSub, types->Int32,   types->Int32);
   graph->registerValidOperation(types->Int64,   aSub, types->Int64,   types->Int64);
   graph->registerValidOperation(types->Float,   aSub, types->Float,   types->Float);
   graph->registerValidOperation(types->Double,  aSub, types->Double,  types->Double);
   graph->registerValidOperation(types->Address, aSub, types->Address, types->Word);
   }


Mul::Mul(Builder * parent, Value * result, Value * left, Value * right)
   : OperationR1V2(aMul, parent, result, left, right)
   {
   }

void
Mul::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->Mul(operandMappers[0]->next(), operandMappers[1]->next()) );
   }

Operation *
Mul::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->operand(0), cloner->operand(1));
   }

void
Mul::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->Int8,    aMul, types->Int8,    types->Int8 );
   graph->registerValidOperation(types->Int16,   aMul, types->Int16,   types->Int16);
   graph->registerValidOperation(types->Int32,   aMul, types->Int32,   types->Int32);
   graph->registerValidOperation(types->Int64,   aMul, types->Int64,   types->Int64);
   graph->registerValidOperation(types->Float,   aMul, types->Float,   types->Float);
   graph->registerValidOperation(types->Double,  aMul, types->Double,  types->Double);
   graph->registerValidOperation(types->Address, aMul, types->Address, types->Word);
   }


IndexAt::IndexAt(Builder * parent, Value * result, Type * pointerType, Value * base, Value * index)
   : OperationR1V2T1(aIndexAt, parent, result, pointerType, base, index)
   {
   }

void
IndexAt::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->IndexAt(typeMappers[0]->next(), operandMappers[0]->next(), operandMappers[1]->next()) );
   }

Operation *
IndexAt::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->type(), cloner->operand(0), cloner->operand(1));
   }

Load::Load(Builder * parent, Value * result, std::string localName)
   : OperationR1S1(aLoad, parent, result, parent->fb()->getSymbol(localName))
   {
   }

void
Load::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Symbol *sym = symbolMappers[0]->next();
   resultMappers[0]->add( b->Load(sym) );
   }

Operation *
Load::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->symbol());
   }

static bool
LoadAtStoreAtExpander(OperationReplacer *replacer)
   {
   Builder *b = replacer->builder();
   Operation *op = replacer->operation();
   assert(op->action() == aLoadAt || op->action() == aStoreAt);

   Type *ptrType = op->operand(0)->type();
   assert(ptrType->isKind<PointerType>());
   Type *baseType = ptrType->refine<PointerType>()->BaseType();

   auto explodedTypes = replacer->explodedTypes();
   if (explodedTypes->find(baseType) != explodedTypes->end())
      {
      // convert to sequence of LoadIndirect/StoreIndirect for each field in layout
      StructType *layout = baseType->layout();
      assert(layout);

      bool isLoad = (op->action() == aLoadAt);
      ValueMapper *baseOperandMapper = replacer->operandMapper(0);
      assert(baseOperandMapper->size() == 1);
      ValueMapper *resultMapper = replacer->resultMapper();
      TypeDictionary *dict = ptrType->owningDictionary();
      Value *basePtr = b->CoercePointer(dict->PointerTo(layout), baseOperandMapper->next());
      for (auto it = layout->FieldsBegin(); it != layout->FieldsEnd(); it++)
         {
         FieldType *fType = it->second;
         if (isLoad)
            resultMapper->add(b->LoadIndirect(fType, basePtr));
         else
            {
            ValueMapper *valueOperandMapper = replacer->operandMapper(1);
            b->StoreIndirect(fType, basePtr, valueOperandMapper->next());
            }
         }
      return true;
      }
   return false;
   }

LoadAt::LoadAt(Builder * parent, Value * result, Type * pointerType, Value * address)
   : OperationR1V1T1(aLoadAt, parent, result, pointerType, address)
   {
   }

void
LoadAt::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->LoadAt(typeMappers[0]->next(), operandMappers[0]->next()) );
   }

Operation *
LoadAt::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->type(), cloner->operand());
   }

bool
LoadAt::expand(OperationReplacer *r) const
   {
   return LoadAtStoreAtExpander(r);
   }

void
LoadField::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Type *t = typeMappers[0]->next();
   assert(t->isKind<FieldType>());
   const FieldType *fieldType = t->refine<FieldType>();
   resultMappers[0]->add( b->LoadField(fieldType, operandMappers[0]->next()) );
   }

Operation *
LoadField::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isKind<FieldType>());
   const FieldType *fieldType = t->refine<FieldType>(t);
   return create(b, cloner->result(), fieldType, cloner->operand());
   }

void
LoadIndirect::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Type *t = typeMappers[0]->next();
   assert(t->isKind<FieldType>());
   const FieldType *fieldType = t->refine<FieldType>(t);
   resultMappers[0]->add( b->LoadIndirect(fieldType, operandMappers[0]->next()) );
   }

Operation *
LoadIndirect::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isKind<FieldType>());
   const FieldType *fieldType = t->refine<FieldType>();
   return create(b, cloner->result(), fieldType, cloner->operand());
   }

Store::Store(Builder * parent, std::string name, Value * value)
   : OperationR0S1V1(aStore, parent, parent->fb()->getSymbol(name), value)
      { }

void
Store::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->Store(symbolMappers[0]->next(), operandMappers[0]->next());
   }

Operation *
Store::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->symbol(), cloner->operand());
   }

StoreAt::StoreAt(Builder * parent, Value * address, Value * value)
   : OperationR0V2(aStoreAt, parent, address, value)
   {
   }

void
StoreAt::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->StoreAt(operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
StoreAt::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->operand(0), cloner->operand(1));
   }

bool
StoreAt::expand(OperationReplacer *r) const
   {
   return LoadAtStoreAtExpander(r);
   }

void
StoreField::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Type *t = typeMappers[0]->next();
   assert(t->isKind<FieldType>());
   const FieldType *fieldType = t->refine<FieldType>();
   b->StoreField(fieldType, operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
StoreField::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isKind<FieldType>());
   const FieldType *fieldType = t->refine<FieldType>();
   return create(b, fieldType, cloner->operand(0), cloner->operand(1));
   }

void
StoreIndirect::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Type *t = typeMappers[0]->next();
   assert(t->isKind<FieldType>());
   const FieldType *fieldType = t->refine<FieldType>();
   b->StoreIndirect(fieldType, operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
StoreIndirect::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isKind<FieldType>());
   FieldType *fieldType = t->refine<FieldType>();
   return create(b, fieldType, cloner->operand(0), cloner->operand(1));
   }

AppendBuilder::AppendBuilder(Builder * parent, Builder * b)
   : OperationB1(aAppendBuilder, parent, b)
   {
   }

void
AppendBuilder::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->AppendBuilder(builderMappers[0]->next());
   }

Operation *
AppendBuilder::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->builder());
   }

Call::Call(Builder * parent, Value *result, Value *function, int32_t numArgs, va_list args)
   : Operation(aCall, parent)
   , _result(result)
   , _function(function)
   , _numArgs(numArgs)
   {
   _args = new Value *[numArgs];
   for (int32_t a=0;a < _numArgs;a++)
      {
      Value *arg = va_arg(args,Value *);
      _args[a] = arg;
      }
   }

Call::Call(Builder * parent, Value *result, Value *function, int32_t numArgs, Value **args)
   : Operation(aCall, parent)
   , _result(result)
   , _function(function)
   , _numArgs(numArgs)
   , _args(args)
   {
   }

void
Call::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Value **arguments = new Value *[_numArgs];
   Value *function = operandMappers[0]->next();
   for (int a=0;a < _numArgs;a++)
      arguments[a] = operandMappers[a+1]->next();
   Value *result = b->Call(function, _numArgs, arguments);
   if (NULL != result)
      resultMappers[0]->add( result );
   }

Operation *
Call::clone(Builder *b, OperationCloner *cloner) const
   {
   Value **arguments = new Value *[_numArgs];
   Value *function = cloner->operand(0);
   for (int a=0;a < _numArgs;a++)
      arguments[a] = cloner->operand(a+1);
   return create(b, function, cloner->result(), _numArgs, arguments);
   }

Goto::Goto(Builder * parent, Builder * b)
   : OperationB1(aGoto, parent, b)
   {
   }

void
Goto::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->Goto(builderMappers[0]->next());
   }

Operation *
Goto::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->builder());
   }

ForLoop::ForLoop(Builder * parent, bool countsUp, LocalSymbol * loopSym,
                                  Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
                                  Value * initial, Value * end, Value * bump)
   : Operation(aForLoop, parent)
   , _countsUp(Literal::create(parent->fb()->dict(), (int8_t)countsUp))
   , _loopSym(loopSym)
   , _loopBody(loopBody)
   , _loopBreak(loopBreak)
   , _loopContinue(loopContinue)
   , _initial(initial)
   , _end(end)
   , _bump(bump)
   { }

ForLoop::ForLoop(Builder * parent, bool countsUp, LocalSymbol * loopSym,
                                  Builder * loopBody, Builder * loopBreak,
                                  Value * initial, Value * end, Value * bump)
   : Operation(aForLoop, parent)
   , _countsUp(Literal::create(parent->fb()->dict(), (int8_t)countsUp))
   , _loopSym(loopSym)
   , _loopBody(loopBody)
   , _loopBreak(loopBreak)
   , _loopContinue(NULL)
   , _initial(initial)
   , _end(end)
   , _bump(bump)
   { }

ForLoop::ForLoop(Builder * parent, bool countsUp, LocalSymbol *loopSym,
                                  Builder * loopBody,
                                  Value * initial, Value * end, Value * bump)
   : Operation(aForLoop, parent)
   , _countsUp(Literal::create(parent->fb()->dict(), (int8_t)countsUp))
   , _loopSym(loopSym)
   , _loopBody(loopBody)
   , _loopBreak(NULL)
   , _loopContinue(NULL)
   , _initial(initial)
   , _end(end)
   , _bump(bump)
   { }

void
ForLoop::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->ForLoop(literalMappers[0]->next()->getInt8(), symbolMappers[0]->next()->refine<LocalSymbol>(),
              builderMappers[0]->next(),
              _loopBreak ? builderMappers[1]->next() : NULL,
              _loopContinue ? builderMappers[2]->next() : NULL,
              operandMappers[0]->next(), operandMappers[1]->next(), operandMappers[2]->next());
   }

Operation *
ForLoop::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->literal(0)->getInt8(), cloner->symbol()->refine<LocalSymbol>(),
               cloner->builder(0),
               _loopBreak ? cloner->builder(1) : NULL,
               _loopContinue ? cloner->builder(2) : NULL,
               cloner->operand(0), cloner->operand(1), cloner->operand(2));
   }

ForLoop *
ForLoop::create(Builder * parent, bool countsUp, LocalSymbol * loopSym,
                           Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
                           Value * initial, Value * end, Value * bump)
   {
   ForLoop * forLoopOp = new ForLoop(parent, countsUp, loopSym,
                                                      loopBody, loopBreak, loopContinue,
                                                      initial, end, bump);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);
   if (loopBreak)
      loopBreak->setTarget()
               ->setBoundness(May)
               ->setBound(forLoopOp)
               ->setBoundness(Must);
   if (loopContinue)
      loopContinue->setTarget()
                  ->setBoundness(May)
                  ->setBound(forLoopOp)
                  ->setBoundness(Must);
   return forLoopOp;
   }

ForLoop *
ForLoop::create(Builder * parent, bool countsUp, LocalSymbol * loopSym,
                           Builder * loopBody, Builder * loopBreak,
                           Value * initial, Value * end, Value * bump)
   {
   ForLoop * forLoopOp = new ForLoop(parent, countsUp, loopSym, loopBody, loopBreak,
                                                      initial, end, bump);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);
   if (loopBreak)
      loopBreak->setTarget()
               ->setBoundness(May)
               ->setBound(forLoopOp)
               ->setBoundness(Must);
   return forLoopOp;
   }

ForLoop *
ForLoop::create(Builder * parent, bool countsUp, LocalSymbol * loopSym,
                           Builder * loopBody, Value * initial, Value * end, Value * bump)
   {
   ForLoop * forLoopOp = new ForLoop(parent, countsUp, loopSym, loopBody, initial, end, bump);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);
   return forLoopOp;
   }

void
ForLoop::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aForLoop, types->Int8  , types->Int8  , types->Int8 );
   graph->registerValidOperation(types->NoType, aForLoop, types->Int16 , types->Int16 , types->Int16);
   graph->registerValidOperation(types->NoType, aForLoop, types->Int32 , types->Int32 , types->Int32);
   graph->registerValidOperation(types->NoType, aForLoop, types->Int64 , types->Int64 , types->Int64);
   graph->registerValidOperation(types->NoType, aForLoop, types->Float , types->Float , types->Float);
   graph->registerValidOperation(types->NoType, aForLoop, types->Double, types->Double, types->Double);
   }

IfCmpGreaterThan::IfCmpGreaterThan(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OperationB1R0V2(aIfCmpGreaterThan, parent, tgt, left, right)
   {
   }

void
IfCmpGreaterThan::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->IfCmpGreaterThan(builderMappers[0]->next(), operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
IfCmpGreaterThan::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->builder(), cloner->operand(0), cloner->operand(1));
   }

void
IfCmpGreaterThan::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterThan, types->Double, types->Double);
   }

IfCmpLessThan::IfCmpLessThan(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OperationB1R0V2(aIfCmpLessThan, parent, tgt, left, right)
   {
   }

void
IfCmpLessThan::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->IfCmpLessThan(builderMappers[0]->next(), operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
IfCmpLessThan::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->builder(), cloner->operand(0), cloner->operand(1));
   }

void
IfCmpLessThan::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpLessThan, types->Double, types->Double);
   }

IfCmpGreaterOrEqual::IfCmpGreaterOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OperationB1R0V2(aIfCmpGreaterOrEqual, parent, tgt, left, right)
   {
   }

void
IfCmpGreaterOrEqual::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->IfCmpGreaterOrEqual(builderMappers[0]->next(), operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
IfCmpGreaterOrEqual::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->builder(), cloner->operand(0), cloner->operand(1));
   }

void
IfCmpGreaterOrEqual::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpGreaterOrEqual, types->Double, types->Double);
   }

IfCmpLessOrEqual::IfCmpLessOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right)
   : OperationB1R0V2(aIfCmpLessOrEqual, parent, tgt, left, right)
   {
   }

void
IfCmpLessOrEqual::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->IfCmpLessOrEqual(builderMappers[0]->next(), operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
IfCmpLessOrEqual::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->builder(), cloner->operand(0), cloner->operand(1));
   }

void
IfCmpLessOrEqual::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int8  , types->Int8  );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int16 , types->Int16 );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int32 , types->Int32 );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Int64 , types->Int64 );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Float , types->Float );
   graph->registerValidOperation(types->NoType, aIfCmpLessOrEqual, types->Double, types->Double);
   }

IfThenElse::IfThenElse(Builder * parent, Builder * thenB, Builder * elseB, Value * cond)
   : OperationB1R0V1(aIfThenElse, parent, thenB, cond)
   , _elseBuilder(elseB)
   {
   }

IfThenElse::IfThenElse(Builder * parent, Builder * thenB, Value * cond)
   : OperationB1R0V1(aIfThenElse, parent, thenB, cond)
   , _elseBuilder(NULL)
   {
   }

void
IfThenElse::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->IfThenElse(builderMappers[0]->next(), _elseBuilder ? builderMappers[1]->next() : NULL, operandMappers[0]->next());
   }

Operation *
IfThenElse::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->builder(0), _elseBuilder ? cloner->builder(1) : NULL, cloner->operand(0));
   }

IfThenElse *
IfThenElse::create(Builder * parent, Builder * thenB, Builder * elseB, Value * cond)
   {
   IfThenElse * ifThenElseOp = new IfThenElse(parent, thenB, elseB, cond);
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

IfThenElse *
IfThenElse::create(Builder * parent, Builder * thenB, Value * cond)
   {
   IfThenElse * ifThenOp = new IfThenElse(parent, thenB, cond);
   thenB->setTarget()
        ->setBoundness(May)
        ->setBound(ifThenOp)
        ->setBoundness(Must);
   return ifThenOp;
   }

void
IfThenElse::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int8);
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int16);
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int32);
   graph->registerValidOperation(types->NoType, aIfThenElse, types->Int64);
   }

Switch::Switch(Builder * parent, Value * selector, Builder *defaultTarget, int numCases, Case ** cases)
   : OperationR0V1(aSwitch, parent, selector)
   , _defaultTarget(defaultTarget)
   , _cases(numCases)
   {
   for (int c=0;c < numCases;c++)
      _cases[c] = cases[c];
   }

Operation *
Switch::clone(Builder *b, Value **results) const
   {
   assert(NULL == results);
   Case **clonedCases = new Case *[numCases()];
   for (int32_t c=0;c < numCases(); c++)
      clonedCases[c] = Case::create(_cases[c]->value(), _cases[c]->builder(), _cases[c]->fallsThrough());
   return new Switch(b, operand(), builder(), numCases(), clonedCases);
   }

Operation *
Switch::clone(Builder *b, Value **results, Value **operands, Builder **builders) const
   {
   assert(NULL == results && operands && builders);
   Case **clonedCases = new Case *[numCases()];
   for (int32_t c=0;c < numCases(); c++)
      clonedCases[c] = Case::create(_cases[c]->value(), builders[c+1], _cases[c]->fallsThrough());
   return new Switch(b, operands[0], builders[0], numCases(), clonedCases);
   }

void
Switch::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const {
    b->Switch(operandMappers[0]->next(), builderMappers[0]->next(), numCases(), (Case **)_cases.data());
}

Operation *
Switch::clone(Builder *b, OperationCloner *cloner) const {
    return create(b, cloner->operand(), cloner->builder(), numCases(), (Case **)_cases.data());
}

void
Switch::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph) {
    graph->registerValidOperation(types->NoType, aSwitch, types->Int32);
}

CreateLocalArray::CreateLocalArray(Builder * parent, Value * result, int32_t numElements, Type * elementType)
   : OperationR1L1T1(aCreateLocalArray, parent, result, Literal::create(parent->fb()->dict(), numElements), elementType) {

}

void
CreateLocalArray::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const {
   resultMappers[0]->add( b->CreateLocalArray(literalMappers[0]->next()->getInt32(), typeMappers[0]->next()) );
}

Operation *
CreateLocalArray::clone(Builder *b, OperationCloner *cloner) const {
    return create(b, cloner->result(), cloner->literal()->getInt32(), cloner->type());
}

CreateLocalStruct::CreateLocalStruct(Builder * parent, Value * result, Type * structType)
   : OperationR1T1(aCreateLocalStruct, parent, result, structType) {

   }

void
CreateLocalStruct::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const {
    resultMappers[0]->add( b->CreateLocalStruct(typeMappers[0]->next()) );
}

Operation *
CreateLocalStruct::clone(Builder *b, OperationCloner *cloner) const {
    return create(b, cloner->result(), cloner->type());
}

#endif

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

