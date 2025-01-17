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
#include "Base/BaseExtension.hpp"
#include "Base/BaseSymbol.hpp"
#include "Base/BaseTypes.hpp"
#include "Base/ControlOperations.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

#define A_UNLESS_B(A,B) (((B) != NULL) ? (B) : (A))


INIT_JBALLOC_REUSECAT(Op_ForLoopUp, Operation)

//
// ForLoopUp
//
Op_ForLoopUp::Op_ForLoopUp(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aForLoopUp, ForLoopBuilder *loopBuilder)
    : Operation(MEM_PASSLOC(a), aForLoopUp, ext, parent)
    , _loopVariable(loopBuilder->loopVariable())
    , _initial(loopBuilder->initialValue())
    , _final(loopBuilder->finalValue())
    , _bump(loopBuilder->bumpValue())
    , _loopBody(ext->BoundBuilder(PASSLOC, parent, this, String(a, "loopBody")))
    , _loopBreak(ext->BoundBuilder(PASSLOC, parent, this, String(a, "loopBreak")))
    , _loopContinue(ext->BoundBuilder(PASSLOC, parent, this, String(a, "loopContinue"))) {

    loopBuilder->setLoopBody(_loopBody);
    loopBuilder->setLoopBreak(_loopBreak);
    loopBuilder->setLoopContinue(_loopContinue);
}

Op_ForLoopUp::Op_ForLoopUp(Allocator *a, const Op_ForLoopUp *source, IRCloner *cloner)
    : Operation(a, source, cloner)
    , _loopVariable(cloner->clonedSymbol(source->_loopVariable))
    , _initial(cloner->clonedValue(source->_initial))
    , _final(cloner->clonedValue(source->_final))
    , _bump(cloner->clonedValue(source->_bump))
    , _loopBody(cloner->clonedBuilder(source->_loopBody))
    , _loopBreak(cloner->clonedBuilder(source->_loopBreak))
    , _loopContinue(cloner->clonedBuilder(source->_loopContinue)) {

}

Op_ForLoopUp::~Op_ForLoopUp() {

}

Operation *
Op_ForLoopUp::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    ForLoopBuilder loopBuilder;
    loopBuilder.setLoopVariable(static_cast<Func::LocalSymbol *>(cloner->symbol()))
               .setInitialValue(cloner->operand(0))
               .setFinalValue(cloner->operand(1))
               .setBumpValue(cloner->operand(2));
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_ForLoopUp(MEM_PASSLOC(mem), this->_ext, b, this->action(), &loopBuilder);
   }

void
Op_ForLoopUp::log(TextLogger & lgr) const {
    lgr << name() << " " << this->_loopVariable << " : " << this->_initial << " to " << this->_final << " by " << this->_bump << " body " << this->_loopBody;
    if (this->_loopBreak)
        lgr << " loopBreak " << this->_loopBreak;
    if (this->_loopContinue)
        lgr << " loopContinue " << this->_loopContinue;
    lgr << lgr.endl();
}


//
// Goto
//
INIT_JBALLOC_REUSECAT(Op_Goto, Operation)

Op_Goto::~Op_Goto() {

}

Operation *
Op_Goto::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Goto(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder());
}

void
Op_Goto::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << lgr.endl();
}


//
// IfCmpEqual
//
INIT_JBALLOC_REUSECAT(Op_IfCmpEqual, Operation)

Op_IfCmpEqual::~Op_IfCmpEqual() {

}

Operation *
Op_IfCmpEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpEqual(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpEqual::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpEqualZero
//
INIT_JBALLOC_REUSECAT(Op_IfCmpEqualZero, Operation)

Op_IfCmpEqualZero::~Op_IfCmpEqualZero() {

}

Operation *
Op_IfCmpEqualZero::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpEqualZero(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand());
}

void
Op_IfCmpEqualZero::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << lgr.endl();
}


//
// IfCmpGreaterThan
//
INIT_JBALLOC_REUSECAT(Op_IfCmpGreaterThan, Operation)

Op_IfCmpGreaterThan::~Op_IfCmpGreaterThan() {

}

Operation *
Op_IfCmpGreaterThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpGreaterThan(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpGreaterThan::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpGreaterOrEqual
//
INIT_JBALLOC_REUSECAT(Op_IfCmpGreaterOrEqual, Operation)

Op_IfCmpGreaterOrEqual::~Op_IfCmpGreaterOrEqual() {

}

Operation *
Op_IfCmpGreaterOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpGreaterOrEqual(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpGreaterOrEqual::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpLessThan
//
INIT_JBALLOC_REUSECAT(Op_IfCmpLessThan, Operation)

Op_IfCmpLessThan::~Op_IfCmpLessThan() {

}

Operation *
Op_IfCmpLessThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpLessThan(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpLessThan::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpLessOrEqual
//
INIT_JBALLOC_REUSECAT(Op_IfCmpLessOrEqual, Operation)

Op_IfCmpLessOrEqual::~Op_IfCmpLessOrEqual() {

}

Operation *
Op_IfCmpLessOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpLessOrEqual(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpLessOrEqual::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpNotEqual
//
INIT_JBALLOC_REUSECAT(Op_IfCmpNotEqual, Operation)

Op_IfCmpNotEqual::~Op_IfCmpNotEqual() {

}

Operation *
Op_IfCmpNotEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpNotEqual(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpNotEqual::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpNotEqualZero
//
INIT_JBALLOC_REUSECAT(Op_IfCmpNotEqualZero, Operation)

Op_IfCmpNotEqualZero::~Op_IfCmpNotEqualZero() {

}

Operation *
Op_IfCmpNotEqualZero::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpNotEqualZero(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand());
}

void
Op_IfCmpNotEqualZero::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << lgr.endl();
}


//
// IfCmpUnsignedGreaterThan
//
INIT_JBALLOC_REUSECAT(Op_IfCmpUnsignedGreaterThan, Operation)

Op_IfCmpUnsignedGreaterThan::~Op_IfCmpUnsignedGreaterThan() {

}

Operation *
Op_IfCmpUnsignedGreaterThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpUnsignedGreaterThan(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedGreaterThan::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpUnsignedGreaterOrEqual
//
INIT_JBALLOC_REUSECAT(Op_IfCmpUnsignedGreaterOrEqual, Operation)

Op_IfCmpUnsignedGreaterOrEqual::~Op_IfCmpUnsignedGreaterOrEqual() {

}

Operation *
Op_IfCmpUnsignedGreaterOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpUnsignedGreaterOrEqual(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedGreaterOrEqual::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpUnsignedLessThan
//
INIT_JBALLOC_REUSECAT(Op_IfCmpUnsignedLessThan, Operation)

Op_IfCmpUnsignedLessThan::~Op_IfCmpUnsignedLessThan() {

}

Operation *
Op_IfCmpUnsignedLessThan::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpUnsignedLessThan(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedLessThan::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfCmpUnsignedLessOrEqual
//
INIT_JBALLOC_REUSECAT(Op_IfCmpUnsignedLessOrEqual, Operation)

Op_IfCmpUnsignedLessOrEqual::~Op_IfCmpUnsignedLessOrEqual() {

}

Operation *
Op_IfCmpUnsignedLessOrEqual::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_IfCmpUnsignedLessOrEqual(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->builder(), cloner->operand(0), cloner->operand(1));
}

void
Op_IfCmpUnsignedLessOrEqual::log(TextLogger & lgr) const {
    lgr << name() << " " << builder() << " " << operand(0) << " " << operand(1) << lgr.endl();
}


//
// IfThenElse
//
INIT_JBALLOC_REUSECAT(Op_IfThenElse, Operation)

Op_IfThenElse::Op_IfThenElse(Allocator *a, const Op_IfThenElse *source, IRCloner *cloner)
    : OperationB1R0V1(a, source, cloner)
    , _elseBuilder(cloner->clonedBuilder(source->_elseBuilder)) {

}

Op_IfThenElse::Op_IfThenElse(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfThenElse, IfThenElseBuilder *bldr)
    : OperationB1R0V1(MEM_PASSLOC(a), aIfThenElse, ext, parent, ext->BoundBuilder(PASSLOC, parent, this, String(a, "thenPath")), bldr->selector())
    , _elseBuilder(ext->BoundBuilder(PASSLOC, parent, this, String(a, "elsePath"))) {

    bldr->setThenPath(thenPath());
    bldr->setElsePath(elsePath());
}

Op_IfThenElse::~Op_IfThenElse() {

}

Operation *
Op_IfThenElse::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    IfThenElseBuilder bldr;
    bldr.setSelector(cloner->operand(0));
    return new (mem) Op_IfThenElse(MEM_PASSLOC(mem), this->_ext, b, this->action(), &bldr);
}

void
Op_IfThenElse::log(TextLogger & lgr) const {
    lgr << name() << " " << operand() << " " << thenPath();
    if (elsePath())
        lgr << " " << elsePath();
    lgr << lgr.endl();
}


//
// SwitchBuilder
//
SwitchBuilder *
SwitchBuilder::addCase(Literal *lv, Builder *builder, bool fallsThrough) {
    Allocator *mem = builder->ir()->mem();
    Case *c = new (mem) Case(mem, lv, builder, fallsThrough);
    _cases->assign(_cases->length(), c);
    return this;
}

SwitchBuilder::~SwitchBuilder() {
    delete _cases;
}

//
// Switch
//
INIT_JBALLOC_REUSECAT(Op_Switch, Operation)

Op_Switch::~Op_Switch() {

}

Op_Switch::Op_Switch(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aSwitch, Value *selector, Builder *defaultBuilder, Array<Case *> *cases)
    : OperationR0V1(MEM_PASSLOC(a), aSwitch, ext, parent, selector)
    , _defaultBuilder(defaultBuilder)
    , _cases(a) {

    Allocator *mem = allocator();
    Array<Case *> *myCases = new (mem) Array<Case*>(mem);
    myCases->assign(cases->length()-1, NULL); // grow it to right size immediately
    uint32_t index = 0;
    for (auto caseIt = _cases.iterator(); caseIt.hasItem(); caseIt++) {
        Case *c = caseIt.item();
        assert(!c->builder()->isBound());
        captureBuilder(c->builder());
        Case *myCase = new (mem) Case(mem, c->literal(), c->builder(), c->fallsThrough());
        myCases->assign(index++, myCase);
    }
}

Op_Switch::Op_Switch(Allocator *a, const Op_Switch *source, IRCloner *cloner)
    : OperationR0V1(a, source, cloner)
    , _defaultBuilder(cloner->clonedBuilder(source->_defaultBuilder))
    , _cases(NULL, a) {

    for (uint32_t i=0;i < _cases.length();i++) {
        Case *c = source->_cases[i];
        _cases.assign(i, cloner->addon<BaseIRClonerAddon>()->clonedCase(c));
    }
}

Operation *
Op_Switch::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    SwitchBuilder bldr(mem);
    bldr.setSelector(cloner->operand(0));
    uint32_t index=0;
    for (auto it=cases();it.hasItem();it++) {
        Case *c = it.item();
        bldr.addCase(cloner->literal(index), cloner->builder(index), c->fallsThrough());
        index++;
    }
    return new (mem) Op_Switch(MEM_PASSLOC(mem), _ext, b, this->action(), cloner->operand(0), cloner->builder(0), bldr.casesArray());
}

void
Op_Switch::log(TextLogger & lgr) const {
    lgr << name() << " " << operand() << lgr.endl();
    LOG_INDENT_REGION(lgr) {
        for (auto it = cases(); it.hasItem(); it++) {
            Case *c = it.item();
            lgr << "[ " << c->literal() << " -> " << c->builder();
            if (c->fallsThrough())
                lgr << " fallsThrough";
        }
        lgr << " ]" << lgr.endl();
    } LOG_OUTDENT
}


BuilderIterator
Op_Switch::builders() {
    Allocator *mem = allocator();
    Builder **array = mem->allocate<Builder *>(_cases.length() + 1);
    uint32_t index = 0;
    for (auto caseIt = _cases.iterator(); caseIt.hasItem(); caseIt++) {
        Case *c = caseIt.item();
        array[index++] = c->builder();
    }
    array[index] = _defaultBuilder;
    BuilderIterator it(mem, array, index);
    mem->deallocate(array); // iterator copied it
    return it;
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
   for (uint32_t a=0;a < _numArgs;a++)
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
   for (uint32_t a=0;a < _numArgs;a++)
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

ForLoop::ForLoop(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
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

ForLoop::ForLoop(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
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

ForLoop::ForLoop(Builder * parent, bool countsUp, Func::LocalSymbol *loopSym,
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
   b->ForLoop(literalMappers[0]->next()->getInt8(), static_cast<Func::LocalSymbol *>(symbolMappers[0]->next()),
              builderMappers[0]->next(),
              _loopBreak ? builderMappers[1]->next() : NULL,
              _loopContinue ? builderMappers[2]->next() : NULL,
              operandMappers[0]->next(), operandMappers[1]->next(), operandMappers[2]->next());
   }

Operation *
ForLoop::clone(Builder *b, OperationCloner *cloner) const
   {
   return create(b, cloner->literal(0)->getInt8(), static_cast<Func::LocalSymbol *>(cloner->symbol()),
               cloner->builder(0),
               _loopBreak ? cloner->builder(1) : NULL,
               _loopContinue ? cloner->builder(2) : NULL,
               cloner->operand(0), cloner->operand(1), cloner->operand(2));
   }

ForLoop *
ForLoop::create(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
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
ForLoop::create(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
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
ForLoop::create(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
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

Switch::Switch(Builder * parent, Value * selector, Builder *defaultTarget, uint32_t numCases, Case ** cases)
   : OperationR0V1(aSwitch, parent, selector)
   , _defaultTarget(defaultTarget)
   , _cases(numCases)
   {
   for (uint32_t c=0;c < numCases;c++)
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
} // namespace JB2
} // namespace OMR

