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
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "TextWriter.hpp"
#include "Value.hpp"

using namespace OMR::JitBuilder;

namespace OMR {
namespace JitBuilder {
namespace Base {

#define A_UNLESS_B(A,B) (((B) != NULL) ? (B) : (A))

//
// Call
//

Op_Call::Op_Call(LOCATION, Extension *ext, Builder * parent, ActionID aCall, Value *result, FunctionSymbol *target, std::va_list & args)
    : OperationR1S1VN(PASSLOC, aCall, ext, parent, result, target, target->functionType()->numParms(), args) {

}


Op_Call::Op_Call(LOCATION, Extension *ext, Builder * parent, ActionID aCall, FunctionSymbol *target, std::va_list & args)
    : OperationR1S1VN(PASSLOC, aCall, ext, parent, NULL, target, target->functionType()->numParms(), args) {

}

Operation *
Op_Call::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Call(PASSLOC, this->_ext, b, this->action(), cloner);
}

void
Op_Call::write(TextWriter & w) const {
    if (_result)
        w << this->_result << " = ";
    w << name() << " " << this->_symbol;
    for (auto a=0;a < _values.size(); a++) {
        w << " " << this->_values[a];
    }
    w << w.endl();
}

void
Op_Call::jbgen(JB1MethodBuilder *j1mb) const {
    FunctionSymbol *funcSym = symbol()->refine<FunctionSymbol>();
    const FunctionType *funcType = funcSym->functionType();
    //j1mb->DefineFunction(funcSym->name(), funcSym->fileName(), funcSym->lineNumber(), funcSym->entryPoint(), funcType->returnType(), funcType->numParms(), funcType->parmTypes());
    if (result())
        j1mb->Call(location(), parent(), result(), funcSym->name(), _values);
    else
        j1mb->Call(location(), parent(), funcSym->name(), _values);
}


//
// ForLoopUp
//
Op_ForLoopUp::Op_ForLoopUp(LOCATION, Extension *ext, Builder * parent, ActionID aForLoopUp, ForLoopBuilder *loopBuilder)
    : Operation(PASSLOC, aForLoopUp, ext, parent)
    , _loopVariable(loopBuilder->loopVariable())
    , _initial(loopBuilder->initialValue())
    , _final(loopBuilder->finalValue())
    , _bump(loopBuilder->bumpValue())
    , _loopBody(A_UNLESS_B(ext->BoundBuilder(PASSLOC, parent, this, std::string("loopBody")), loopBuilder->loopBody()))
    , _loopBreak(A_UNLESS_B(ext->BoundBuilder(PASSLOC, parent, this, std::string("loopBreak")), loopBuilder->loopBreak()))
    , _loopContinue(A_UNLESS_B(ext->BoundBuilder(PASSLOC, parent, this, std::string("loopContinue")), loopBuilder->loopContinue())) {

    if (loopBuilder->loopBody() == NULL)
        loopBuilder->setLoopBody(_loopBody);
    _loopBody->setBound(this);

    if (loopBuilder->loopBreak() == NULL)
        loopBuilder->setLoopBreak(_loopBreak);
    _loopBreak->setBound(this);

    if (loopBuilder->loopContinue() == NULL)
        loopBuilder->setLoopContinue(_loopContinue);
    _loopContinue->setBound(this);
}

Operation *
Op_ForLoopUp::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    ForLoopBuilder loopBuilder;
    loopBuilder.setLoopVariable(static_cast<LocalSymbol *>(cloner->symbol()))
               ->setInitialValue(cloner->operand(0))
               ->setFinalValue(cloner->operand(1))
               ->setBumpValue(cloner->operand(2))
               ->setLoopBody(cloner->builder(0))
               ->setLoopBreak(cloner->builder(1))
               ->setLoopContinue(cloner->builder(2));
    return new Op_ForLoopUp(PASSLOC, this->_ext, b, this->action(), &loopBuilder);
   }

void
Op_ForLoopUp::write(TextWriter & w) const {
    w << name() << " " << this->_loopVariable << " : " << this->_initial << " to " << this->_final << " by " << this->_bump << " body " << this->_loopBody;
    if (this->_loopBreak)
        w << " loopBreak " << this->_loopBreak;
    if (this->_loopContinue)
        w << " loopContinue " << this->_loopContinue;
    w << w.endl();
}

void
Op_ForLoopUp::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->ForLoopUp(location(), parent(), this->_loopVariable, this->_initial, this->_final, this->_bump, this->_loopBody, this->_loopBreak, this->_loopContinue);
}


//
// Goto
//
Operation *
Op_Goto::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Goto(PASSLOC, this->_ext, b, this->action(), cloner->builder());
}

void
Op_Goto::write(TextWriter & w) const {
    w << name() << " " << builder() << w.endl();
}

void
Op_Goto::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Goto(location(), parent(), builder());
}

//
// IfCmpEqual
//
Operation *
Op_IfCmpEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpEqual(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpEqual::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpEqual::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpEqual(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpEqualZero
//
Operation *
Op_IfCmpEqualZero::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpEqualZero(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand());
}

void
Op_IfCmpEqualZero::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << w.endl();
}

void
Op_IfCmpEqualZero::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpEqualZero(location(), parent(), builder(), operand());
}

//
// IfCmpGreaterThan
//
Operation *
Op_IfCmpGreaterThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpGreaterThan(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpGreaterThan::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpGreaterThan::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpGreaterThan(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpGreaterOrEqual
//
Operation *
Op_IfCmpGreaterOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpGreaterOrEqual(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpGreaterOrEqual::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpGreaterOrEqual::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpGreaterOrEqual(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpLessThan
//
Operation *
Op_IfCmpLessThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpLessThan(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpLessThan::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpLessThan::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpLessThan(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpLessOrEqual
//
Operation *
Op_IfCmpLessOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpLessOrEqual(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpLessOrEqual::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpLessOrEqual::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpLessOrEqual(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpNotEqual
//
Operation *
Op_IfCmpNotEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpNotEqual(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpNotEqual::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpNotEqual::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpNotEqual(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpNotEqualZero
//
Operation *
Op_IfCmpNotEqualZero::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpNotEqualZero(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand());
}

void
Op_IfCmpNotEqualZero::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << w.endl();
}

void
Op_IfCmpNotEqualZero::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpNotEqualZero(location(), parent(), builder(), operand());
}

//
// IfCmpUnsignedGreaterThan
//
Operation *
Op_IfCmpUnsignedGreaterThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpUnsignedGreaterThan(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedGreaterThan::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpUnsignedGreaterThan::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpUnsignedGreaterThan(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpUnsignedGreaterOrEqual
//
Operation *
Op_IfCmpUnsignedGreaterOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpUnsignedGreaterOrEqual(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedGreaterOrEqual::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpUnsignedGreaterOrEqual::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpUnsignedGreaterOrEqual(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpUnsignedLessThan
//
Operation *
Op_IfCmpUnsignedLessThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpUnsignedLessThan(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedLessThan::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpUnsignedLessThan::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpUnsignedLessThan(location(), parent(), builder(), operand(0), operand(1));
}

//
// IfCmpUnsignedLessOrEqual
//
Operation *
Op_IfCmpUnsignedLessOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_IfCmpUnsignedLessOrEqual(PASSLOC, this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedLessOrEqual::write(TextWriter & w) const {
    w << name() << " " << builder() << " " << operand(0) << " " << operand(1) << w.endl();
}

void
Op_IfCmpUnsignedLessOrEqual::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->IfCmpUnsignedLessOrEqual(location(), parent(), builder(), operand(0), operand(1));
}

//
// Return
//
Op_Return::Op_Return(LOCATION, Extension *ext, Builder * parent, ActionID aReturn)
    : Operation(PASSLOC, aReturn, ext, parent), _value(NULL) {
    parent->setControlReachesEnd(false);
}

Op_Return::Op_Return(LOCATION, Extension *ext, Builder * parent, ActionID aReturn, Value * v)
    : Operation(PASSLOC, aReturn, ext, parent), _value(v) {
    parent->setControlReachesEnd(false);
}

Operation *
Op_Return::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    if (NULL != _value)
        return new Op_Return(PASSLOC, this->_ext, b, this->action(), cloner->operand());
    else
        return new Op_Return(PASSLOC, this->_ext, b, this->action());
}

void
Op_Return::write(TextWriter & w) const {
    w << name();
    if (_value)
        w << " " << operand();
    w << w.endl();
}

void
Op_Return::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Return(location(), parent(), operand());
}


#if 0
// keep around and handy during migration

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

#endif

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

