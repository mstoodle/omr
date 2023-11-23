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

#ifndef CONTROLOPERATIONS_INCL
#define CONTROLOPERATIONS_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/BaseIRAddon.hpp"
#include "Base/BaseIRClonerAddon.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

class BaseExtension;

class Op_Goto : public OperationB1 {
    JBALLOC_(Op_Goto)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_Goto(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aGoto, Builder *target)
        : OperationB1(MEM_PASSLOC(a), aGoto, ext, parent, target) {

    }
    IRCLONER_SUPPORT(Op_Goto, OperationB1)
};

class Op_IfCmpEqual : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpEqual)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpEqual(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpEqual, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpEqual, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpEqual, OperationB1R0V2)
};

class Op_IfCmpEqualZero : public OperationB1R0V1 {
    JBALLOC_(Op_IfCmpEqualZero)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpEqualZero(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpEqualZero, Builder *target, Value *value)
        : OperationB1R0V1(MEM_PASSLOC(a), aIfCmpEqualZero, ext, parent, target, value) {

    }
    IRCLONER_SUPPORT(Op_IfCmpEqualZero, OperationB1R0V1)
};

class Op_IfCmpGreaterThan : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpGreaterThan)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpGreaterThan(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpGreaterThan, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpGreaterThan, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpGreaterThan, OperationB1R0V2)
};

class Op_IfCmpGreaterOrEqual : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpGreaterOrEqual)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpGreaterOrEqual(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpGreaterOrEqual, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpGreaterOrEqual, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpGreaterOrEqual, OperationB1R0V2)
};

class Op_IfCmpLessThan : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpLessThan)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpLessThan(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpLessThan, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpLessThan, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpLessThan, OperationB1R0V2)
};

class Op_IfCmpLessOrEqual : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpLessOrEqual)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpLessOrEqual(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpLessOrEqual, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpLessOrEqual, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpLessOrEqual, OperationB1R0V2)
};

class Op_IfCmpNotEqual : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpNotEqual)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpNotEqual(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpNotEqual, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpNotEqual, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpNotEqual, OperationB1R0V2)
};

class Op_IfCmpNotEqualZero : public OperationB1R0V1 {
    JBALLOC_(Op_IfCmpNotEqualZero)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpNotEqualZero(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpNotEqualZero, Builder *target, Value *value)
        : OperationB1R0V1(MEM_PASSLOC(a), aIfCmpNotEqualZero, ext, parent, target, value) {

    }
    IRCLONER_SUPPORT(Op_IfCmpNotEqualZero, OperationB1R0V1)
};

class Op_IfCmpUnsignedGreaterThan : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpUnsignedGreaterThan)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpUnsignedGreaterThan(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpUnsignedGreaterThan, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpUnsignedGreaterThan, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpUnsignedGreaterThan, OperationB1R0V2)
};

class Op_IfCmpUnsignedGreaterOrEqual : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpUnsignedGreaterOrEqual)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpUnsignedGreaterOrEqual(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpUnsignedGreaterOrEqual, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpUnsignedGreaterOrEqual, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpUnsignedGreaterOrEqual, OperationB1R0V2)
};

class Op_IfCmpUnsignedLessThan : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpUnsignedLessThan)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpUnsignedLessThan(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpUnsignedLessThan, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpUnsignedLessThan, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpUnsignedLessThan, OperationB1R0V2)
};

class Op_IfCmpUnsignedLessOrEqual : public OperationB1R0V2 {
    JBALLOC_(Op_IfCmpUnsignedLessOrEqual)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

protected:
    Op_IfCmpUnsignedLessOrEqual(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfCmpUnsignedLessOrEqual, Builder *target, Value *left, Value *right)
        : OperationB1R0V2(MEM_PASSLOC(a), aIfCmpUnsignedLessOrEqual, ext, parent, target, left, right) {

    }
    IRCLONER_SUPPORT(Op_IfCmpUnsignedLessOrEqual, OperationB1R0V2)
};

class Op_ForLoopUp : public Operation {
    JBALLOC_(Op_ForLoopUp)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

    virtual size_t numSymbols() const { return 1; }
    virtual Symbol * symbol(uint32_t i=0) const {
        if (i == 0) return _loopVariable;
        return NULL;
    }
    virtual SymbolIterator symbols() {
        return SymbolIterator(allocator(), _loopVariable);
    }

    virtual size_t numOperands() const { return 3; }
    virtual Value * operand(uint32_t i=0) const {
        if (i == 0) return _initial;
        else if (i == 1) return _final;
        else if (i == 2) return _bump;
        return NULL;
    }
    virtual ValueIterator operands() {
        return ValueIterator(allocator(), _initial, _final, _bump);
    }

    virtual size_t numBuilders() const {
        return 3;
    }
    virtual Builder * builder(uint32_t i=0) const {
        if (i == 0) return _loopBody;
        else if (i == 1) return _loopBreak;
        else if (i == 2) return _loopContinue;
        return NULL;
    }
    virtual BuilderIterator builders() {
        return BuilderIterator(allocator(), _loopBody, _loopBreak, _loopContinue);
    }

    Symbol *loopVariable() const { return _loopVariable; }
    Value *initial() const { return _initial; }
    Value *final() const { return _final; }
    Value *bump() const { return _bump; }
    Builder *loopBody() const { return _loopBody; }
    Builder *loopBreak() const { return _loopBreak; }
    Builder *loopContinue() const { return _loopContinue; }

protected:
    Op_ForLoopUp(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aForLoopUp, ForLoopBuilder *loopBuilder);
    Op_ForLoopUp(Allocator *a, const Op_ForLoopUp *source, IRCloner *cloner);
    virtual Operation *clone(Allocator *mem, IRCloner *cloner) const {
        return new (mem) Op_ForLoopUp(mem, this, cloner);
    }

    Symbol *_loopVariable;
    Value * _initial;
    Value * _final;
    Value * _bump;
    Builder *_loopBody;
    Builder *_loopBreak;
    Builder *_loopContinue;
    };


class Op_IfThenElse : public OperationB1R0V1 {
    JBALLOC_(Op_IfThenElse)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

    virtual size_t size() const { return sizeof(Op_IfThenElse); }

    virtual Builder * thenPath() const { return _builder; }
    virtual Builder * elsePath() const { return _elseBuilder; }

    virtual size_t numBuilders() const { return 2; }
    virtual Builder * builder(uint32_t i=0) const {
        if (i == 0)
            return _builder;
        else if (i == 1)
            return _elseBuilder;
        return NULL;
    }
    virtual BuilderIterator builders() {
        return BuilderIterator(allocator(), _builder, _elseBuilder);
    }

protected:
    Op_IfThenElse(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfThenElse, IfThenElseBuilder * bldr);
    Op_IfThenElse(Allocator *a, const Op_IfThenElse *source, IRCloner *cloner)
        : OperationB1R0V1(a, source, cloner)
        , _elseBuilder(cloner->clonedBuilder(source->_elseBuilder)) {

    }
    virtual Operation *clone(Allocator *mem, IRCloner *cloner) const {
        return new (mem) Op_IfThenElse(mem, this, cloner);
    }

    Builder * _elseBuilder;
    };

class Case : Allocatable {
public:
    Case(Allocator *a, Literal *lv, Builder *builder, bool fallsThrough)
        : Allocatable(a)
        , _id(builder->ir()->addon<BaseIRAddon>()->getCaseID())
        , _lv(lv)
        , _builder(builder)
        , _fallsThrough(fallsThrough) {
    
    }
    Case(Allocator *a, const Case *source, IRCloner *cloner)
        : Allocatable(a)
        , _lv(cloner->clonedLiteral(source->_lv))
        , _builder(cloner->clonedBuilder(source->_builder))
        , _fallsThrough(source->_fallsThrough) {

    }
    virtual Case *clone(Allocator *mem, IRCloner *cloner) const {
        return new (mem) Case(mem, this, cloner);
    }

    CaseID id() const { return _id; }
    Literal *literal() const { return _lv; }
    Builder *builder() const { return _builder; }
    bool fallsThrough() const { return _fallsThrough; }

private:
    CaseID _id;
    Literal *_lv;
    Builder *_builder;
    bool _fallsThrough;
};

class Op_Switch;

class SwitchBuilder {
    friend class BaseExtension;
    friend class Op_Switch;

public:
    SwitchBuilder(Allocator *a)
        : _selector(NULL)
        , _cases(new (a) Array<Case *>(a, a))
        , _defaultBuilder(NULL) {

    }
    ~SwitchBuilder();

    SwitchBuilder *setSelector(Value *selector) { _selector = selector; return this; }
    SwitchBuilder *setDefaultBuilder(Builder *builder) { _defaultBuilder = builder; return this; }
    SwitchBuilder *addCase(Literal *lv, Builder *builder, bool fallsThrough);

protected:
    Value *selector() const { return _selector; }
    Array<Case *>::ForwardIterator cases() { return _cases->iterator(); }
    Array<Case *> *casesArray() const { return _cases; }
    Builder *defaultBuilder() const { return _defaultBuilder; }

private:
    Value *_selector;
    Array<Case *> *_cases;
    Builder *_defaultBuilder;
};

class Op_Switch : public OperationR0V1 {
    JBALLOC_(Op_Switch)

    friend class BaseExtension;
public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;
    virtual void log(TextLogger & lgr) const;

    virtual size_t size() const { return sizeof(Op_Switch); }

    virtual Value * selector() const { return _value; }
    virtual Builder *defaultBuilder() const { return _defaultBuilder; }
    virtual size_t numBuilders() const { return 1 + _cases.length(); }
    virtual Builder *builder(size_t i=0) const {
        if (i < _cases.length())
            return _cases[i]->builder();
        return NULL;
    }
    virtual BuilderIterator builders();

    virtual size_t numCases() const  { return _cases.length(); }
    virtual Array<Case *>::ForwardIterator cases() const { return _cases.constIterator(); }

    protected:
    Op_Switch(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aSwitch, Value *selector, Builder *defaultBuilder, Array<Case *> *cases);
    Op_Switch(Allocator *a, const Op_Switch *source, IRCloner *cloner)
        : OperationR0V1(a, source, cloner)
        , _defaultBuilder(cloner->clonedBuilder(source->_defaultBuilder))
        , _cases(NULL, a) {

        for (uint32_t i=0;i < _cases.length();i++) {
            Case *c = source->_cases[i];
            _cases.assign(i, cloner->addon<BaseIRClonerAddon>()->clonedCase(c));
        }
    }
    virtual Operation *clone(Allocator *mem, IRCloner *cloner) const;

    Builder *_defaultBuilder;
    Array<Case *> _cases;
    };

#if 0
// keep handy during migration

class AppendBuilder : public OperationB1
    {
    public:
    static AppendBuilder * create(Builder * parent, Builder * b)
        { return new AppendBuilder(parent, b); }
    
    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, builder(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && NULL == operands && builders);
        return create(b, builders[0]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    AppendBuilder(Builder * parent, Builder * b);
    };

class Call : public Operation
    {
    public:
    static Call * create(Builder * parent, Value *function, size_t numArgs, va_list args)
        {
        Call *call = new Call(parent, function, NULL, numArgs, args);
        return call;
        }
    static Call * create(Builder * parent, Value *function, Value *result, size_t numArgs, va_list args)
        {
        Call *call = new Call(parent, function, result, numArgs, args);
        return call;
        }
    static Call * create(Builder * parent, Value *function, size_t numArgs, Value **args)
        {
        Call *call = new Call(parent, function, NULL, numArgs, args);
        return call;
        }
    static Call * create(Builder * parent, Value *function, Value *result, size_t numArgs, Value **args)
        {
        Call *call = new Call(parent, function, result, numArgs, args);
        return call;
        }

    virtual size_t size() const { return sizeof(Call); }
    
    Value *function() const  { return _function; }
    size_t numArguments() const { return _numArgs; }
    Value *argument(size_t a) const
        {
        if (a < _numArgs)
            return _args[a];
        return NULL;
        }

    virtual size_t numOperands() const    { return _numArgs+1; }
    virtual Value * operand(uint32_t i=0) const
        {
        if (i == 0)
            return _function;
        else if (i >= 1 && i <= _numArgs)
            return _args[i-1];
        return NULL;
        }
    virtual ValueIterator OperandsBegin()
        {
        ValueIterator it1(_function);
        ValueIterator it2(_args, _numArgs);
        it2.prepend(it1);
        return it2;
        }

    virtual size_t numResults() const             { return (_result != NULL) ? 1 : 0; }
    virtual Value * result(uint32_t i=0) const
        {
        if (i == 0) return _result; // may return NULL if there is no result
        return NULL;
        }
    virtual ValueIterator ResultsBegin()
        {
        if (_result)
            return ValueIterator(_result);
        return ResultsEnd();
        }

    virtual Operation * clone(Builder *b, Value **results) const
        {
        Value **cloneArgs = new Value *[_numArgs];
        for (size_t a=0;a < _numArgs;a++)
            cloneArgs[a] = _args[a];
        if (_result)
            {
            assert(results);
            return new Call(b, operand(0), results[0], _numArgs, cloneArgs);
            }
        else
            {
            assert(NULL == results);
            return new Call(b, operand(0), NULL, _numArgs, cloneArgs);
            }
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        if (_result)
            {
            assert(results && operands && NULL == builders);
            return new Call(b, operands[0], results[0], _numArgs-1, operands+1);
            }
        else
            {
            assert(NULL == results && operands && NULL == builders);
            return new Call(b, operands[0], NULL, _numArgs-1, operands+1);
            }
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    Call(Builder * parent, Value *result, Value *function, size_t numArgs, va_list args);
    Call(Builder * parent, Value *result, Value *function, size_t numArgs, Value **args);

    Value *_function;
    Value *_result;
    size_t _numArgs;
    Value **_args;
    };

class Goto : public OperationB1
    {
    public:
    static Goto * create(Builder * parent, Builder * b)
        { return new Goto(parent, b); }
    
    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, builder(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && NULL == operands && builders);
        return create(b, builders[0]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    Goto(Builder * parent, Builder * b);
    };

class ForLoop : public Operation
    {
    public:
    virtual size_t size() const { return sizeof(ForLoop); }
    static ForLoop * create(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
                                    Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
                                    Value * initial, Value * end, Value * bump);
    static ForLoop * create(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
                                    Builder * loopBody, Builder * loopBreak,
                                    Value * initial, Value * end, Value * bump);
    static ForLoop * create(Builder * parent, bool countsUp, Func::LocalSymbol * loopSym,
                                    Builder * loopBody, Value * initial, Value * end, Value * bump);

    static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

    virtual bool countsUp() const                              { return (bool)(_countsUp->getInt8()); }
    virtual Func::LocalSymbol *getLoopSymbol() const             { return _loopSym; }

    virtual Value * getInitial() const                        { return _initial; }
    virtual Value * getEnd() const                             { return _end; }
    virtual Value * getBump() const                            { return _bump; }

    virtual size_t numLiterals() const                      { return 1; }
    virtual Literal *literal(uint32_t i=0) const
        {
        if (i == 0) return _countsUp;
        return NULL;
        }
    virtual LiteralIterator litIterator()                 { return LiteralIterator(_countsUp); }

    virtual size_t numSymbols() const                        { return 1; }
    virtual Symbol *symbol(uint32_t i=0) const
        {
        if (i == 0) return _loopSym;
        return NULL;
        }
    virtual SymbolIterator SymbolsBegin()                    { return SymbolIterator(_loopSym); }

    virtual size_t numOperands() const                      { return 3; }
    virtual Value * operand(uint32_t i=0) const
        {
        if (i == 0) return _initial;
        if (i == 1) return _end;
        if (i == 2) return _bump;
        return NULL;
        }
    virtual ValueIterator OperandsBegin()                    { return ValueIterator(_initial, _end, _bump); }

    virtual Builder * getBody() const                         { return _loopBody; }
    virtual Builder * getBreak() const                        { return _loopBreak; }
    virtual Builder * getContinue() const                    { return _loopContinue; }
    virtual size_t numBuilders() const                      { return 1 + (_loopBreak ? 1 : 0) + (_loopContinue ? 1 : 0); }

    virtual Builder * builder(uint32_t i=0) const
        {
        if (i == 0) return _loopBody;
        if (i == 1) return _loopBreak;
        if (i == 2) return _loopContinue;
        return NULL;
        }

    virtual BuilderIterator BuildersBegin()
        {
        if (_loopContinue) return BuilderIterator(_loopBody, _loopBreak, _loopContinue);
        if (_loopBreak)     return BuilderIterator(_loopBody, _loopBreak);
        return BuilderIterator(_loopBody);
        }

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, literal(0)->getInt8(), _loopSym, builder(0), builder(1), builder(2), operand(0), operand(1), operand(2));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && builders);
        return create(b, literal(0)->getInt8(), _loopSym, builders[0], _loopBreak ? builders[1] : NULL, _loopContinue ? builders[2] : NULL, operands[0], operands[1], operands[2]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    ForLoop(Builder * parent, bool countsUp, Func::LocalSymbol *loopSym,
              Builder * loopBody, Builder * loopBreak, Builder * loopContinue,
              Value * initial, Value * end, Value * bump);
    ForLoop(Builder * parent, bool countsUp, Func::LocalSymbol *loopSym,
              Builder * loopBody, Builder * loopBreak,
              Value * initial, Value * end, Value * bump);
    ForLoop(Builder * parent, bool countsUp, Func::LocalSymbol *loopSym,
              Builder * loopBody,
              Value * initial, Value * end, Value * bump);

    Literal *_countsUp;
    Func::LocalSymbol *_loopSym;

    Builder * _loopBody;
    Builder * _loopBreak;
    Builder * _loopContinue;

    Value * _initial;
    Value * _end;
    Value * _bump;
    };

class IfCmpGreaterThan : public OperationB1R0V2
    {
    public:
    static IfCmpGreaterThan * create(Builder * parent, Builder * tgt, Value * left, Value * right)
        { return new IfCmpGreaterThan(parent, tgt, left, right); }

    static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, builder(0), operand(0), operand(1));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && builders);
        return create(b, builders[0], operands[0], operands[1]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    IfCmpGreaterThan(Builder * parent, Builder * tgt, Value * left, Value * right);
    };

class IfCmpLessThan : public OperationB1R0V2
    {
    public:
    static IfCmpLessThan * create(Builder * parent, Builder * tgt, Value * left, Value * right)
        { return new IfCmpLessThan(parent, tgt, left, right); }

    static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, builder(0), operand(0), operand(1));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && builders);
        return create(b, builders[0], operands[0], operands[1]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    IfCmpLessThan(Builder * parent, Builder * tgt, Value * left, Value * right);
    };

class IfCmpGreaterOrEqual : public OperationB1R0V2
    {
    public:
    static IfCmpGreaterOrEqual * create(Builder * parent, Builder * tgt, Value * left, Value * right)
        { return new IfCmpGreaterOrEqual(parent, tgt, left, right); }

    static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, builder(0), operand(0), operand(1));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && builders);
        return create(b, builders[0], operands[0], operands[1]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    IfCmpGreaterOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right);
    };

class IfCmpLessOrEqual : public OperationB1R0V2
    {
    public:
    static IfCmpLessOrEqual * create(Builder * parent, Builder * tgt, Value * left, Value * right)
        { return new IfCmpLessOrEqual(parent, tgt, left, right); }

    static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, builder(0), operand(0), operand(1));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && builders);
        return create(b, builders[0], operands[0], operands[1]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    IfCmpLessOrEqual(Builder * parent, Builder * tgt, Value * left, Value * right);
    };

class IfThenElse : public OperationB1R0V1
    {
    public:
    virtual size_t size() const { return sizeof(IfThenElse); }
    static IfThenElse * create(Builder * parent, Builder * thenB, Builder * elseB, Value * cond);
    static IfThenElse * create(Builder * parent, Builder * thenB, Value * cond);

    static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

    virtual Builder * getThenBuilder() const { return _builder; }
    virtual Builder * getElseBuilder() const { return _elseBuilder; }

    virtual size_t numBuilders() const { return _elseBuilder ? 2 : 1; }
    virtual Builder * builder(uint32_t i=0) const
        {
        if (i == 0)
            return _builder;
        else if (i == 1 && _elseBuilder)
            return _elseBuilder;
        return NULL;
        }
    virtual BuilderIterator BuildersBegin()
        {
        if (_elseBuilder)
            return BuilderIterator(_builder, _elseBuilder);
        return BuilderIterator(_builder);
        }

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, builder(0), builder(1), operand(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && builders);
        return create(b, builders[0], _elseBuilder ? builders[1] : NULL, operands[0]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    IfThenElse(Builder * parent, Builder * thenB, Builder * elseB, Value * cond);
    IfThenElse(Builder * parent, Builder * thenB, Value * cond);

    Builder * _elseBuilder;
    };

class Switch : public OperationR0V1
    {
    public:
    virtual size_t size() const { return sizeof(Switch); }
    static Switch * create(Builder * parent, Value *selector, Builder *defaultTarget, uint32_t numCases, Case ** cases)
        { return new Switch(parent, selector, defaultTarget, numCases, cases); }

    virtual Value * getSelector() const { return _value; }

    virtual size_t numBuilders() const { return 1 + _cases.length(); }
    virtual Builder *builder(size_t i=0) const
        {
        if (i == 0)
            return _defaultTarget;
        else if (i-1 < _cases.length())
            return _cases[i-1]->builder();
        return NULL;
        }
    virtual BuilderIterator BuildersBegin()
        {
        BuilderIterator it(_defaultTarget);
        for (auto cIt = CasesBegin(); cIt != CasesEnd(); cIt++)
            it.push_back((*cIt)->builder());
        return BuilderIterator(it);
        }

    virtual size_t numCases() const  { return _cases.length(); }
    virtual CaseIterator CasesBegin() { return CaseIterator(_cases); }

    static void initializeTypeProductions(TypeDictionary * types, TypeGraph * graph);

    virtual Operation * clone(Builder *b, Value **results) const;
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const;
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    Switch(Builder * parent, Value *selector, Builder *defaultCase, uint32_t numCases, Case ** cases);

    Builder *_defaultTarget;
    Array<Case *> _cases;
    };

class CreateLocalArray : public OperationR1L1T1
    {
    public:
    static CreateLocalArray * create(Builder * parent, Value * result, size_t numElements, Type * elementType)
        { return new CreateLocalArray(parent, result, numElements, elementType); }
    
    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(results);
        return create(b, results[0], literal(0)->getInt32(), type(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(results && NULL == operands && NULL == builders);
        return create(b, results[0], literal(0)->getInt32(), type(0));
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    CreateLocalArray(Builder * parent, Value * result, size_t numElements, Type * elementType);
    };

class CreateLocalStruct : public OperationR1T1
    {
    public:
    static CreateLocalStruct * create(Builder * parent, Value * result, Type * structType)
        { return new CreateLocalStruct(parent, result, structType); }
    
    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(results);
        return create(b, results[0], type(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(results && NULL == operands && NULL == builders);
        return create(b, results[0], type(0));
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    CreateLocalStruct(Builder * parent, Value * result, Type * structType);
    };

#endif

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(CONTROLOPERATIONS_INCL)
