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
#include "Builder.hpp"
//#include "Case.hpp"
#include "Compilation.hpp"
#include "Extension.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "Symbol.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"
#include "TextLogger.hpp"

namespace OMR {
namespace JitBuilder {


INIT_JBALLOC(Operation)

Operation::Operation(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Operation *next, Operation *prev)
    : Allocatable(a)
    , _id(parent->ir()->getOperationID())
    , _ext(ext)
    , _parent(parent)
    , _next(next)
    , _prev(prev)
    , _action(action)
    , _name(ext->actionName(action))
    , _location(parent->location())
    , _creationLocation(PASSLOC) {

    } 

Operation::Operation(Allocator *a, const Operation *source, IRCloner *cloner)
    : Allocatable(a)
    , _id(source->_id)
    , _ext(source->_ext)
    , _parent(cloner->clonedBuilder(source->_parent))
    , _next(cloner->clonedOperation(source->_next))
    , _prev(cloner->clonedOperation(source->_prev))
    , _action(source->_action)
    , _name(source->_name)
    , _location(cloner->clonedLocation(source->_location))
    , _creationLocation(source->_creationLocation) {

    } 

Operation::~Operation() {
}

Operation *
Operation::setParent(Builder * newParent) {
    _parent = newParent;
    return static_cast<Operation *>(this);
}

Operation *
Operation::setLocation(Location *location) {
    _location = location;
    return static_cast<Operation *>(this);
}

Operation *
Operation::unlink() {
    Operation *p = _prev;
    if (_prev) {
        _prev->_next = _next;
    } else {
        _parent->_firstOperation = _next;
    }
    if (_next) {
        _next->_prev = _prev;
    } else {
        _parent->_lastOperation = _prev;
    }

    _parent->_operationCount--;
    _next = _prev = NULL;
    _parent = NULL;
    return p;
}

Operation *
Operation::replace(Builder *b) {
    Operation *first = b->firstOperation();
    Operation *last = b->lastOperation();

    last->setNext(_next);
    if (_next) {
        _next->setPrev(last);
    } else {
	_parent->_lastOperation = last;
    }

    first->setPrev(_prev);
    if (_prev) {
        _prev->setNext(first);
    } else {
        _parent->_firstOperation = first;
    }

    // update parent operation count with b's operations minus 1 for removing "this"
    _parent->_operationCount += b->_operationCount - 1;

    // this is no longer part of parent
    _parent = NULL;
    _next = _prev = NULL;

    return first->prev();
}

void
Operation::addToBuilder(Extension *ext, Builder *b, Operation *op) {
    ext->addOperation(b, op);
}

void
Operation::registerDefinition(Value *result) {
    result->addDefinition(this);
}

void
Operation::captureBuilder(Builder *b) {
    assert(!b->isBound());
    b->setBound(this);
}
void
Operation::logFull(TextLogger & lgr) const {
    lgr.indent() << _parent << "!o" << _id << " : ";
    log(lgr);
}

void
Operation::log(TextLogger & lgr) const {
    lgr << this->name() << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR0S1, Operation)

OperationR0S1::~OperationR0S1() {
}

void
OperationR0S1::log(TextLogger & lgr) const {
    lgr << this->name() << " " << this->_symbol << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR0S1V1, Operation)

OperationR0S1V1::~OperationR0S1V1() {
}

void
OperationR0S1V1::log(TextLogger & lgr) const {
    lgr << this->name() << " " << this->_symbol << " " << this->_value << lgr.endl();
}


INIT_JBALLOC_REUSECAT(OperationR0V1, Operation)

OperationR0V1::~OperationR0V1() {
}

void
OperationR0V1::log(TextLogger & lgr) const {
    lgr << this->name() << " " << this->_value << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR0V2, Operation)

OperationR0V2::~OperationR0V2() {
}
void
OperationR0V2::log(TextLogger & lgr) const {
    lgr << this->name() << " " << this->_left << " " << this->_right << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR0T1, Operation)

OperationR0T1::~OperationR0T1() {
}

INIT_JBALLOC_REUSECAT(OperationR0T1V2, Operation)

OperationR0T1V2::~OperationR0T1V2() {
}

void
OperationR0T1V2::log(TextLogger & lgr) const {
    lgr << this->name() << " ";
    this->_type->logType(lgr);
    lgr << " " << this->_base << " " << this->_value << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR0S1VN, Operation)

OperationR0S1VN::OperationR0S1VN(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, OperationCloner * cloner)
    : OperationR0S1(MEM_PASSLOC(a), action, ext, parent, cloner->symbol()) {

    _numValues = cloner->numOperands();
    if (_numValues > 0) {
        _values = new Value *[_numValues];
        for (auto a=0;a < cloner->numOperands(); a++)
            _values[a] = (cloner->operand(a));
    } else {
        _values = NULL;
    }
}

OperationR0S1VN::~OperationR0S1VN() {
    if (_values != NULL) {
        allocator()->deallocate(_values);
    }
}

void
OperationR0S1VN::log(TextLogger & lgr) const {
    lgr << this->name() << " " << this->symbol();
    for (auto a=0;a < this->numOperands(); a++)
        lgr << " " << operand(a);
    lgr << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1, Operation)

OperationR1::OperationR1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result)
    : Operation(MEM_PASSLOC(a), action, ext, parent)
    , _result(result) {

    registerDefinition(result);
}

OperationR1::~OperationR1() {
    // _result can be cleared during destruction by e.g. Op_MergeDef because only one def should delete _result
    // Only the original definition of the value will delete it
    if (_result)
        delete _result;
}

INIT_JBALLOC_REUSECAT(OperationR1L1, Operation)

OperationR1L1::~OperationR1L1() {
}

void
OperationR1L1::log(TextLogger &lgr) const {
    lgr << this->_result << " = " << name() << " " << literal() << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1S1, Operation)

OperationR1S1::~OperationR1S1() {
}

void
OperationR1S1::log(TextLogger & lgr) const {
    lgr << this->_result << " = " << this->name() << " " << this->_symbol << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1T1, Operation)

OperationR1T1::~OperationR1T1() {
}

void
OperationR1T1::log(TextLogger & lgr) const {
    lgr << this->_result << " = " << this->name() << " ";
    this->_type->logType(lgr);
    lgr << lgr.endl();
}

OperationR1V1::~OperationR1V1() {
}

void
OperationR1V1::log(TextLogger & lgr) const {
    lgr << this->_result << " = " << this->name() << " " << this->_value << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1L1T1, Operation)

OperationR1L1T1::~OperationR1L1T1() {
}

void
OperationR1L1T1::log(TextLogger & lgr) const {
    lgr << this->_result << " = " << this->name() << " " << this->_lv << " ";
    this->_elementType->logType(lgr);
    lgr << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1V1T1, Operation)

OperationR1V1T1::~OperationR1V1T1() {
}

void
OperationR1V1T1::log(TextLogger & lgr) const {
    lgr << this->_result << " = " << this->name() << " ";
    this->_type->logType(lgr);
    lgr << " " << this->_value << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1V2, Operation)

OperationR1V2::~OperationR1V2() {
}

void
OperationR1V2::log(TextLogger & lgr) const {
    lgr << this->_result << " = " << this->name() << " " << this->_left << " " << this->_right << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1V2T1, Operation)

OperationR1V2T1::~OperationR1V2T1() {
}

void
OperationR1V2T1::log(TextLogger & lgr) const {
    lgr << this->_result << " = " << this->name() << " ";
    this->_type->logType(lgr);
    lgr << " " << this->_left << " " << this->_right << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationR1S1VN, Operation)

OperationR1S1VN::OperationR1S1VN(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, OperationCloner * cloner)
    : OperationR1S1(MEM_PASSLOC(a), action, ext, parent, cloner->result(), cloner->symbol()) {

    _numValues = cloner->numOperands();
    if (_numValues > 0) {
        _values = new Value *[_numValues];
        for (auto a=0;a < cloner->numOperands(); a++)
            _values[a] = (cloner->operand(a));
    } else {
        _values = NULL;
    }
}

OperationR1S1VN::~OperationR1S1VN() {
    if (_values != NULL) {
        allocator()->deallocate(_values);
    }
}

void
OperationR1S1VN::log(TextLogger & lgr) const {
    lgr << this->_result << " = ";
    lgr << this->name() << " " << this->symbol();
    for (auto a=0;a < this->numOperands(); a++)
        lgr << " " << operand(a);
    lgr << lgr.endl();
}

INIT_JBALLOC_REUSECAT(OperationB1, Operation)

OperationB1::~OperationB1() {

}

INIT_JBALLOC_REUSECAT(OperationB1R0V1, Operation)

OperationB1R0V1::~OperationB1R0V1() {

}

INIT_JBALLOC_REUSECAT(OperationB1R0V2, Operation)

OperationB1R0V2::~OperationB1R0V2() {

}

//
// Core operations
//

INIT_JBALLOC_REUSECAT(Op_AppendBuilder, Operation)

Op_AppendBuilder::~Op_AppendBuilder() {
}

Op_AppendBuilder::Op_AppendBuilder(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aAppendBuilder, Builder *b)
    : OperationB1(MEM_PASSLOC(a), aAppendBuilder, ext, parent, b) {
}

Operation *
Op_AppendBuilder::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_AppendBuilder(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder());
}


INIT_JBALLOC_REUSECAT(Op_MergeDef, Operation)

Op_MergeDef::~Op_MergeDef() {
    this->_result = NULL; // hacky but prevents OperationR1 from deleting the definition which is shared with another operation
}

Op_MergeDef::Op_MergeDef(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aMergeDef, Value *existingDef, Value *newDef)
    : OperationR1V1(MEM_PASSLOC(a), aMergeDef, ext, parent, existingDef, newDef) {
}

Operation *
Op_MergeDef::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_MergeDef(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->operand());
}

#if 0
// keep this stuff handy during the migration

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

static bool
LoadAtStoreAtExpander(OperationReplacer *replacer)
   {
   Builder *b = replacer->builder();
   Operation *op = replacer->operation();
   assert(op->action() == aLoadAt || op->action() == aStoreAt);

   Type *ptrType = op->operand(0)->type();
   assert(ptrType->isPointer());
   TypeDictionary *dict = ptrType->owningDictionary();
   Type *baseType = static_cast<PointerType *>(ptrType)->BaseType();

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

bool
LoadAt::expand(OperationReplacer *r) const
   {
   return LoadAtStoreAtExpander(r);
   }

void
LoadField::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Type *t = typeMappers[0]->next();
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
   resultMappers[0]->add( b->LoadField(fieldType, operandMappers[0]->next()) );
   }

Operation *
LoadField::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
   return create(b, cloner->result(), fieldType, cloner->operand());
   }

void
LoadIndirect::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Type *t = typeMappers[0]->next();
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
   resultMappers[0]->add( b->LoadIndirect(fieldType, operandMappers[0]->next()) );
   }

Operation *
LoadIndirect::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
   return create(b, cloner->result(), fieldType, cloner->operand());
   }

Store::Store(Builder * parent, String name, Value * value)
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
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
   b->StoreField(fieldType, operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
StoreField::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
   return create(b, fieldType, cloner->operand(0), cloner->operand(1));
   }

void
StoreIndirect::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   Type *t = typeMappers[0]->next();
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
   b->StoreIndirect(fieldType, operandMappers[0]->next(), operandMappers[1]->next());
   }

Operation *
StoreIndirect::clone(Builder *b, OperationCloner *cloner) const
   {
   Type *t = cloner->type();
   assert(t->isField());
   FieldType *fieldType = static_cast<FieldType *>(t);
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

Call::Call(Builder * parent, Value *result, Value *function, size_t numArgs, va_list args)
   : Operation(aCall, parent)
   , _result(result)
   , _function(function)
   , _numArgs(numArgs)
   {
   _args = new Value *[numArgs];
   for (size_t a=0;a < _numArgs;a++)
      {
      Value *arg = va_arg(args,Value *);
      _args[a] = arg;
      }
   }

Call::Call(Builder * parent, Value *result, Value *function, size_t numArgs, Value **args)
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
   b->ForLoop(literalMappers[0]->next()->getInt8(), static_cast<LocalSymbol *>(symbolMappers[0]->next()),
              builderMappers[0]->next(),
              _loopBreak ? builderMappers[1]->next() : NULL,
              _loopContinue ? builderMappers[2]->next() : NULL,
              operandMappers[0]->next(), operandMappers[1]->next(), operandMappers[2]->next());
   }

Operation *
ForLoop::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->literal(0)->getInt8(), static_cast<LocalSymbol *>(cloner->symbol()),
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

Return::Return(Builder * parent)
   : Operation(aReturn, parent), _value(NULL)
   {
   }

Return::Return(Builder * parent, Value * v)
   : Operation(aReturn, parent), _value(v)
   {
   }

void
Return::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   if (NULL != _value)
      b->Return(operandMappers[0]->next());
   else
      b->Return();
   }

Operation *
Return::clone(Builder *b, OperationCloner *cloner) const
   {
   if (NULL != _value)
      return create(b, cloner->operand());
   else
      return create(b);
   }

void
Return::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aReturn, types->Int8);
   graph->registerValidOperation(types->NoType, aReturn, types->Int16);
   graph->registerValidOperation(types->NoType, aReturn, types->Int32);
   graph->registerValidOperation(types->NoType, aReturn, types->Int64);
   graph->registerValidOperation(types->NoType, aReturn, types->Float);
   graph->registerValidOperation(types->NoType, aReturn, types->Double);
   graph->registerValidOperation(types->NoType, aReturn, types->Address);
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
   for (size_t c=0;c < numCases(); c++)
      clonedCases[c] = Case::create(_cases[c]->value(), _cases[c]->builder(), _cases[c]->fallsThrough());
   return new Switch(b, operand(), builder(), numCases(), clonedCases);
   }

Operation *
Switch::clone(Builder *b, Value **results, Value **operands, Builder **builders) const
   {
   assert(NULL == results && operands && builders);
   Case **clonedCases = new Case *[numCases()];
   for (size_t c=0;c < numCases(); c++)
      clonedCases[c] = Case::create(_cases[c]->value(), builders[c+1], _cases[c]->fallsThrough());
   return new Switch(b, operands[0], builders[0], numCases(), clonedCases);
   }

void
Switch::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   b->Switch(operandMappers[0]->next(), builderMappers[0]->next(), numCases(), (Case **)_cases.data());
   }

Operation *
Switch::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->operand(), cloner->builder(), numCases(), (Case **)_cases.data());
   }

void
Switch::initializeTypeProductions(TypeDictionary * types, TypeGraph * graph)
   {
   graph->registerValidOperation(types->NoType, aSwitch, types->Int32);
   }

CreateLocalArray::CreateLocalArray(Builder * parent, Value * result, size_t numElements, Type * elementType)
   : OperationR1L1T1(aCreateLocalArray, parent, result, Literal::create(parent->fb()->dict(), numElements), elementType)
   {
   }

void
CreateLocalArray::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->CreateLocalArray(literalMappers[0]->next()->getInt32(), typeMappers[0]->next()) );
   }

Operation *
CreateLocalArray::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->literal()->getInt32(), cloner->type());
   }

CreateLocalStruct::CreateLocalStruct(Builder * parent, Value * result, Type * structType)
   : OperationR1T1(aCreateLocalStruct, parent, result, structType)
   {
   }

void
CreateLocalStruct::cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMappers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const
   {
   resultMappers[0]->add( b->CreateLocalStruct(typeMappers[0]->next()) );
   }

Operation *
CreateLocalStruct::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->result(), cloner->type());
   }
#endif

} // namespace JitBuilder
} // namespace OMR

