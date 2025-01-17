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
namespace JB2 {
namespace Base {

class BaseExtension;

DECL_OPERATION_CLASS(Op_Goto, OperationB1, BaseExtension,
    Op_Goto(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aGoto, Builder *target)
        : OperationB1(MEM_PASSLOC(a), aGoto, ext, parent, target) { }
    public: \
        virtual void log(TextLogger & lgr) const; \
)

// This macro simplifies creating an IfCmp operation that compares one value to something specific (so
// single operand, hence IfCmp1) and conditionally branches to a specified builder .
#define DECL_IFCMP1_OPERATION_CLASS(C) \
    DECL_OPERATION_CLASS(C,OperationB1R0V1,BaseExtension, \
        C(MEM_LOCATION(a), Extension *ext, Builder *parent, ActionID action, Builder *target, Value *value) \
            : OperationB1R0V1(MEM_PASSLOC(a), action, ext, parent, target, value) { } \
    public: \
        virtual void log(TextLogger & lgr) const; \
    )

DECL_IFCMP1_OPERATION_CLASS(Op_IfCmpEqualZero)
DECL_IFCMP1_OPERATION_CLASS(Op_IfCmpNotEqualZero)


// This macro simplifies creating an IfCmp operation that compares two values (dual operand, hence IfCmp2)
// and conditionally branches to a specified builder .
#define DECL_IFCMP2_OPERATION_CLASS(C) \
    DECL_OPERATION_CLASS(C,OperationB1R0V2,BaseExtension, \
        C(MEM_LOCATION(a), Extension *ext, Builder *parent, ActionID action, Builder *target, Value *left, Value *right) \
            : OperationB1R0V2(MEM_PASSLOC(a), action, ext, parent, target, left, right) { } \
    public: \
        virtual void log(TextLogger & lgr) const; \
    )

DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpEqual)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpGreaterThan)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpGreaterOrEqual)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpLessThan)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpLessOrEqual)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpNotEqual)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpUnsignedGreaterThan)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpUnsignedGreaterOrEqual)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpUnsignedLessThan)
DECL_IFCMP2_OPERATION_CLASS(Op_IfCmpUnsignedLessOrEqual)

DECL_OPERATION_CLASS_WITH_STATE(Op_ForLoopUp, Operation, BaseExtension,
    Op_ForLoopUp(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aForLoopUp, ForLoopBuilder *loopBuilder);

    Symbol *_loopVariable;
    Value * _initial;
    Value * _final;
    Value * _bump;
    Builder *_loopBody;
    Builder *_loopBreak;
    Builder *_loopContinue;

public:
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
)


DECL_OPERATION_CLASS_WITH_STATE(Op_IfThenElse, OperationB1R0V1, BaseExtension,
    Op_IfThenElse(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIfThenElse, IfThenElseBuilder * bldr);
    Builder * _elseBuilder;
public:
    virtual void log(TextLogger & lgr) const;

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

)

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

DECL_OPERATION_CLASS_WITH_STATE(Op_Switch, OperationR0V1, BaseExtension,
    Op_Switch(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aSwitch, Value *selector, Builder *defaultBuilder, Array<Case *> *cases);
    Builder *_defaultBuilder;
    Array<Case *> _cases;
public:
    virtual void log(TextLogger & lgr) const;

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
)

} // namespace Base
} // namespace JB2
} // namespace OMR

#endif // !defined(CONTROLOPERATIONS_INCL)
