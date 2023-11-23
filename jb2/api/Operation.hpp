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

#ifndef OPERATION_INCL
#define OPERATION_INCL

#include <cstdarg>
#include "common.hpp"
#include "CreateLoc.hpp"
#include "IRCloner.hpp"
#include "Mapper.hpp"
#include "String.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Extension;
class IRCloner;
class Literal;
class Location;
class Operation;
class OperationCloner;
class OperationReplacer;
class TextLogger;
class Type;
class TypeDictionary;
class Value;

// Operation defines an interface to all kinds of operations, it cannot itself be instantiated
// Operation classes defined for specific templates (which can be instantiated) follow

class Operation : public Allocatable {
    JBALLOC_(Operation)

    friend class Builder;
    friend class Extension;
    friend class IRCloner;
    friend class Transformer;

public:
    OperationID id() const      { return _id; }
    ActionID action() const     { return _action; }
    Extension *ext() const      { return _ext; }
    Builder * parent() const    { return _parent; }
    Location * location() const { return _location; }

    Operation *next() const { return _next; }
    Operation *prev() const { return _prev; }
    Operation *unlink();
    Operation *replace(Builder *b);

    virtual size_t size() const    { return sizeof(Operation); }
    virtual bool isDynamic() const { return false; }

    virtual BuilderIterator builders()           { return BuilderIterator(); }
    virtual size_t numBuilders() const           { return 0; }
    virtual Builder *builder(uint32_t i=0) const { return NULL; }

    virtual LiteralIterator literals()            { return LiteralIterator(); }
    virtual size_t numLiterals() const            { return 0; }
    virtual Literal * literal(uint32_t i=0) const { assert(0); return 0; }

    virtual SymbolIterator symbols()           { return SymbolIterator(); }
    virtual size_t numSymbols() const          { return 0; }
    virtual Symbol *symbol(uint32_t i=0) const { assert(0); return 0; }

    virtual ValueIterator operands()            { return ValueIterator(); }
    virtual size_t numOperands() const          { return 0; }
    virtual Value * operand(uint32_t i=0) const { return NULL; }

    virtual ValueIterator results()            { return ValueIterator(); }
    virtual size_t numResults() const          { return 0; }
    virtual Value * result(uint32_t i=0) const { return NULL; }
 
    // Change to mayReadShadow() and mustReadShadow
    virtual SymbolIterator readSymbols()            { return SymbolIterator(); }
    virtual size_t numReadSymbols() const           { return 0; }
    virtual Symbol * readSymbol(uint32_t i=0) const { return NULL; }

    // Change to mayWriteShadow() and mustWriteShadow
    virtual SymbolIterator writtenSymbols()            { return SymbolIterator(); }
    virtual size_t numWrittenSymbols() const           { return 0; }
    virtual Symbol * writtenSymbol(uint32_t i=0) const { return NULL; }

#ifdef CASES_BECOME_CORE
    virtual CaseIterator CasesBegin()          { return CaseIterator(); }
    virtual CaseIterator CasesEnd()            { return caseEndIterator; }
    virtual size_t numCases() const            { return 0; }
    virtual Case * getCase(uint32_t i=0) const { return NULL; }
#endif

    virtual TypeIterator types()                  { return TypeIterator(); }
    virtual size_t numTypes() const               { return 0; }
    virtual const Type * type(uint32_t i=0) const { return NULL; }

    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const = 0;

    virtual bool hasExpander() const                       { return false; }
    virtual bool expand(OperationReplacer *replacer) const { return false; }

    void logFull(TextLogger & log) const;
    const String & name() const { return _name; }
    virtual void log(TextLogger & log) const;

protected:
    Operation(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Operation *next=NULL, Operation *prev=NULL);
    Operation(Allocator *a, const Operation *source, IRCloner *cloner);

    virtual Operation *clone(Allocator *a, IRCloner *cloner) const = 0;

    Operation * setParent(Builder * newParent);
    Operation * setLocation(Location *location);
    Operation * setNext(Operation *next) { _next = next; return this; }
    Operation * setPrev(Operation *prev) { _prev = prev; return this; }

    void captureBuilder(Builder *b);

    void registerDefinition(Value *result);

    static void addToBuilder(Extension *ext, Builder *b, Operation *op);

    OperationID _id;
    Extension * _ext;
    Builder * _parent;
    Operation *_next;
    Operation *_prev;
    ActionID _action;
    const String _name;
    Location * _location;
    CreateLocation _creationLocation;
};


// Following are "structural" classes that are used to hold results(R), literals(L),
// operand values (V), Symbols(S), and builders (B) for different kinds
// of Operations. The name signifies the structure using the designator letters
// listed above. So the structural Operation class with one result and two operand
// values is called OperationR1V2. Operation sub classes then derive from these
// structural classes and can add some semantically relevant services.
// These classes are abstract so they cannot be accidentally created directly.
class OperationR0S1 : public Operation {
    JBALLOC_(OperationR0S1)

public:
    virtual size_t size() const { return sizeof(OperationR0S1); }

    virtual size_t numSymbols() const { return 1; }
    virtual Symbol *symbol(uint32_t i=0) const {
        if (i == 0) return _symbol;
        return NULL;
    }
    virtual SymbolIterator symbols() { return SymbolIterator(allocator(), _symbol); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR0S1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Symbol *symbol)
        : Operation(MEM_PASSLOC(a), action, ext, parent)
        , _symbol(symbol) {

    }
    OperationR0S1(Allocator *a, const OperationR0S1 *source, IRCloner *cloner)
        : Operation(a, source, cloner)
        , _symbol(cloner->clonedSymbol(source->_symbol)) {

    }

    Symbol *_symbol;
};

class OperationR0S1V1 : public OperationR0S1 {
    JBALLOC_(OperationR0S1V1)

public:
    virtual size_t size() const { return sizeof(OperationR0S1V1); }

    virtual size_t numOperands() const { return 1; }
    virtual Value * operand(uint32_t i=0) const {
        if (i == 0) return _value;
        return NULL;
    }
    virtual ValueIterator operands() { return ValueIterator(allocator(), _value); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR0S1V1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Symbol *symbol, Value * value)
        : OperationR0S1(MEM_PASSLOC(a), action, ext, parent, symbol)
        , _value(value) {

    }
    OperationR0S1V1(Allocator *a, const OperationR0S1V1 *source, IRCloner *cloner)
        : OperationR0S1(a, source, cloner)
        , _value(cloner->clonedValue(source->_value)) {

    }

    Value * _value;
};

class OperationR0T1 : public Operation {
    JBALLOC_(OperationR0T1)

    public:
    virtual size_t size() const { return sizeof(OperationR0T1); }

    virtual size_t numTypes() const { return 1; }
    virtual const Type *type(uint32_t i=0) const {
        if (i == 0) return _type;
        return NULL;
    }
    virtual TypeIterator types()       { return TypeIterator(allocator(), _type); }

    protected:
    OperationR0T1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder *parent, const Type *type)
        : Operation(MEM_PASSLOC(a), action, ext, parent)
        , _type(type) {
    }
    OperationR0T1(Allocator *a, const OperationR0T1 *source, IRCloner *cloner)
        : Operation(a, source, cloner)
        , _type(cloner->clonedType(source->_type)) {

    }

    const Type *_type;
};

class OperationR0T1V2 : public OperationR0T1 {
    JBALLOC_(OperationR0T1V2)

public:
    virtual size_t size() const        { return sizeof(OperationR0T1V2); }
    virtual size_t numOperands() const { return 2; }
    virtual Value * operand(uint32_t i=0) const {
        if (i == 0) return _base;
        if (i == 1) return _value;
        return NULL;
    }
    virtual ValueIterator operands() { return ValueIterator(allocator(), _base, _value); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR0T1V2(MEM_LOCATION(a), ActionID action, Extension *ext, Builder *parent, const Type *type, Value * base, Value * value)
        : OperationR0T1(MEM_PASSLOC(a), action, ext, parent, type)
        , _base(base)
        , _value(value) {
    }
    OperationR0T1V2(Allocator *a, const OperationR0T1V2 *source, IRCloner *cloner)
        : OperationR0T1(a, source, cloner)
        , _base(cloner->clonedValue(source->_base))
        , _value(cloner->clonedValue(source->_value)) {

    }

    Value *_base;
    Value *_value;
};

class OperationR0V1 : public Operation {
    JBALLOC_(OperationR0V1)

public:
    virtual size_t size() const        { return sizeof(OperationR0V1); }
    virtual size_t numOperands() const { return 1; }
    virtual Value * operand(uint32_t i=0) const {
        if (i == 0) return _value;
        return NULL;
    }

    virtual ValueIterator operands() { return ValueIterator(allocator(), _value); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR0V1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * value)
        : Operation(MEM_PASSLOC(a), action, ext, parent)
        , _value(value)
        { }
    OperationR0V1(Allocator *a, const OperationR0V1 *source, IRCloner *cloner)
        : Operation(a, source, cloner)
        , _value(cloner->clonedValue(source->_value)) {

    }

    Value * _value;
};

class OperationR0V2 : public Operation {
    JBALLOC_(OperationR0V2)

public:
    virtual size_t size() const        { return sizeof(OperationR0V2); }
    virtual size_t numOperands() const { return 2; }
    virtual Value * operand(uint32_t i=0) const {
        if (i == 0) return _left;
        if (i == 1) return _right;
        return NULL;
    }
    virtual Value * getLeft() const  { return _left; }
    virtual Value * getRight() const { return _right; }

    virtual ValueIterator operands() { return ValueIterator(allocator(), _left, _right); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR0V2(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * left, Value * right)
        : Operation(MEM_PASSLOC(a), action, ext, parent)
        , _left(left)
        , _right(right) {
    }
    OperationR0V2(Allocator *a, const OperationR0V2 *source, IRCloner *cloner)
        : Operation(a, source, cloner)
        , _left(cloner->clonedValue(source->_left))
        , _right(cloner->clonedValue(source->_right)) {

    }

    Value * _left;
    Value * _right;
};

class OperationR0S1VN : public OperationR0S1 {
    JBALLOC_(OperationR0S1VN)

public:
    virtual size_t size() const { return sizeof(OperationR0S1VN); }

    virtual size_t numOperands() const { return _numValues; }
    virtual Value * operand(uint32_t i=0) const {
        if (i < _numValues) return _values[i];
        return NULL;
    }

    virtual ValueIterator operands() { return ValueIterator(allocator(), _values, _numValues); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR0S1VN(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Symbol *symbol, size_t numArgs, std::va_list & args)
        : OperationR0S1(MEM_PASSLOC(a), action, ext, parent, symbol) {

        _numValues = numArgs;
        if (_numValues > 0) {
            _values = a->allocate<Value *>(numArgs);
            for (auto a=0;a < numArgs;a++) {
                _values[a] = (va_arg(args, Value *));
            }
        } else {
            _values = NULL;
        }
    }
    OperationR0S1VN(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, OperationCloner * cloner);
    OperationR0S1VN(Allocator *a, const OperationR0S1VN *source, IRCloner *cloner)
        : OperationR0S1(a, source, cloner) {

        _numValues = source->_numValues;
        if (_numValues > 0) {
            _values = a->allocate<Value *>(_numValues);
            for (auto a=0;a < _numValues;a++) {
                _values[a] = cloner->clonedValue(source->_values[a]);
            }
        } else {
            _values = NULL;
        }
    }

    size_t _numValues;
    Value ** _values;
};

class OperationR1 : public Operation {
    JBALLOC_(OperationR1)

public:
    virtual size_t size() const       { return sizeof(OperationR1); }
    virtual size_t numResults() const { return 1; }
    virtual Value * result(uint32_t i=0) const {
        if (i == 0) return _result;
        return NULL;
    }
    virtual ValueIterator results() { return ValueIterator(allocator(), _result); }

protected:
    OperationR1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result);
    OperationR1(Allocator *a, const OperationR1 *source, IRCloner *cloner)
        : Operation(a, source, cloner)
        , _result(cloner->clonedValue(source->_result)) {

    }

     Value * _result;
};

class OperationR1L1 : public OperationR1 {
    JBALLOC_(OperationR1L1)

public:
    virtual size_t numLiterals() const { return 1; }
    virtual Literal *literal(uint32_t i=0) const {
        if (i == 0) return _lv;
        return NULL;
    }
    virtual LiteralIterator literals() { return LiteralIterator(allocator(), _lv); }
    virtual void log(TextLogger & log) const;

protected:
    OperationR1L1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, Literal *lv)
         : OperationR1(MEM_PASSLOC(a), action, ext, parent, result)
         , _lv(lv)
         { }
    OperationR1L1(Allocator *a, const OperationR1L1 *source, IRCloner *cloner)
        : OperationR1(a, source, cloner)
        , _lv(cloner->clonedLiteral(source->_lv)) {

    }

    Literal *_lv;
};

class OperationR1L1T1 : public OperationR1L1 {
    JBALLOC_(OperationR1L1T1)

public:
    virtual size_t size() const     { return sizeof(OperationR1L1T1); }
    virtual size_t numTypes() const { return 1; }
    virtual const Type * type(uint32_t i=0) const {
        if (i == 0) return _elementType;
        return NULL;
    }
    virtual TypeIterator types() { return TypeIterator(allocator(), _elementType); }
    virtual void log(TextLogger & log) const;

protected:
    OperationR1L1T1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, Literal *numElements, const Type *elementType)
        : OperationR1L1(MEM_PASSLOC(a), action, ext, parent, result, numElements)
        , _elementType(elementType) {

    }
    OperationR1L1T1(Allocator *a, const OperationR1L1T1 *source, IRCloner *cloner)
        : OperationR1L1(a, source, cloner)
        , _elementType(cloner->clonedType(source->_elementType)) {

    }

    const Type *_elementType;
};

class OperationR1S1 : public OperationR1 {
    JBALLOC_(OperationR1S1)

public:
    virtual size_t numSymbols() const { return 1; }
    virtual Symbol *symbol(uint32_t i=0) const {
        if (i == 0) return _symbol;
        return NULL;
    }
    virtual SymbolIterator symbols() { return SymbolIterator(allocator(), _symbol); }
    virtual void log(TextLogger & log) const;

protected:
    OperationR1S1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, Symbol *symbol)
        : OperationR1(MEM_PASSLOC(a), action, ext, parent, result)
        , _symbol(symbol) {
    }
    OperationR1S1(Allocator *a, const OperationR1S1 *source, IRCloner *cloner)
        : OperationR1(a, source, cloner)
        , _symbol(cloner->clonedSymbol(source->_symbol)) {

    }

    Symbol *_symbol;
};

class OperationR1T1 : public OperationR1 {
    JBALLOC_(OperationR1T1)

public:
    virtual size_t size() const     { return sizeof(OperationR1T1); }
    virtual size_t numTypes() const { return 1; }
    virtual const Type * type(uint32_t i=0) const {
        if (i == 0) return _type;
        return NULL;
    }
    virtual TypeIterator types() { return TypeIterator(allocator(), _type); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR1T1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, const Type * t)
        : OperationR1(MEM_PASSLOC(a), action, ext, parent, result)
        , _type(t) {

    }
    OperationR1T1(Allocator *a, const OperationR1T1 *source, IRCloner *cloner)
        : OperationR1(a, source, cloner)
        , _type(cloner->clonedType(source->_type)) {

    }

    const Type * _type;
};

class OperationR1V1 : public OperationR1 {
    JBALLOC_(OperationR1V1)

public:
    virtual size_t size() const        { return sizeof(OperationR1V1); }
    virtual size_t numOperands() const { return 1; }
    virtual Value * operand(uint32_t i=0) const {
        if (i == 0) return _value;
        return NULL;
    }

    virtual ValueIterator operands() { return ValueIterator(allocator(), _value); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR1V1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, Value * value)
        : OperationR1(MEM_PASSLOC(a), action, ext, parent, result)
        , _value(value) {
    }
    OperationR1V1(Allocator *a, const OperationR1V1 *source, IRCloner *cloner)
        : OperationR1(a, source, cloner)
        , _value(cloner->clonedValue(source->_value)) {

    }

    Value * _value;
};

class OperationR1V1T1 : public OperationR1V1 {
    JBALLOC_(OperationR1V1T1)

public:
    virtual size_t size() const     { return sizeof(OperationR1V1T1); }
    virtual size_t numTypes() const { return 1; }
    virtual const Type * type(uint32_t i=0) const {
        if (i == 0) return _type;
        return NULL;
    }

    virtual TypeIterator types() { return TypeIterator(allocator(), _type); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR1V1T1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, const Type * t, Value * v)
        : OperationR1V1(MEM_PASSLOC(a), action, ext, parent, result, v)
        , _type(t) {

    }
    OperationR1V1T1(Allocator *a, const OperationR1V1T1 *source, IRCloner *cloner)
        : OperationR1V1(a, source, cloner)
        , _type(cloner->clonedType(source->_type)) {

    }

     const Type * _type;
};

class OperationR1V2 : public OperationR1 {
    JBALLOC_(OperationR1V2)

public:
    virtual size_t size() const        { return sizeof(OperationR1V2); }
    virtual size_t numOperands() const { return 2; }
    virtual Value * operand(uint32_t i=0) const {
        if (i == 0) return _left;
        if (i == 1) return _right;
        return NULL;
    }
    virtual Value * getLeft() const  { return _left; }
    virtual Value * getRight() const { return _right; }

    virtual ValueIterator operands() { return ValueIterator(allocator(), _left, _right); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR1V2(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, Value * left, Value * right)
        : OperationR1(MEM_PASSLOC(a), action, ext, parent, result)
        , _left(left)
        , _right(right) {

    }
    OperationR1V2(Allocator *a, const OperationR1V2 *source, IRCloner *cloner)
        : OperationR1(a, source, cloner)
        , _left(cloner->clonedValue(source->_left))
        , _right(cloner->clonedValue(source->_right)) {

    }

    Value * _left;
    Value * _right;
};

class OperationR1V2T1 : public OperationR1V2 {
    JBALLOC_(OperationR1V2T1)

public:
    virtual size_t size() const     { return sizeof(OperationR1V2T1); }
    virtual size_t numTypes() const { return 1; }
    virtual const Type * type(uint32_t i=0) const {
        if (i == 0) return _type;
        return NULL;
    }
    virtual TypeIterator types() { return TypeIterator(allocator(), _type); }

    virtual Value * getAddress() const { return _left; }
    virtual Value * getValue() const { return _right; }

    virtual void log(TextLogger & log) const;

protected:
    OperationR1V2T1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value * result, const Type * t, Value * addr, Value * v)
        : OperationR1V2(MEM_PASSLOC(a), action, ext, parent, result, addr, v)
        , _type(t) {

    }
    OperationR1V2T1(Allocator *a, const OperationR1V2T1 *source, IRCloner *cloner)
        : OperationR1V2(a, source, cloner)
        , _type(cloner->clonedType(source->_type)) {

    }

    const Type * _type;
};

class OperationR1S1VN : public OperationR1S1 {
    JBALLOC_(OperationR1S1VN)

public:
    virtual size_t size() const { return sizeof(OperationR1S1VN); }

    virtual size_t numOperands() const { return _numValues; }
    virtual Value * operand(uint32_t i=0) const {
        if (i < _numValues) return _values[i];
        return NULL;
    }

    virtual ValueIterator operands() { return ValueIterator(allocator(), _numValues, _values); }

    virtual void log(TextLogger & log) const;

protected:
    OperationR1S1VN(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Value *result, Symbol *symbol, size_t numArgs, std::va_list & args)
        : OperationR1S1(MEM_PASSLOC(a), action, ext, parent, result, symbol) {

        _numValues = numArgs;
        if (_numValues > 0) {
            _values = new Value *[numArgs];
            for (auto a=0;a < numArgs;a++) {
                _values[a] = (va_arg(args, Value *));
            }
        } else {
            _values = NULL;
        }
    }
    OperationR1S1VN(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, OperationCloner * cloner);
    OperationR1S1VN(Allocator *a, const OperationR1S1VN *source, IRCloner *cloner)
        : OperationR1S1(a, source, cloner) {

        _numValues = source->_numValues;
        if (_numValues > 0) {
            _values = a->allocate<Value *>(_numValues);
            for (auto a=0;a < _numValues;a++) {
                _values[a] = cloner->clonedValue(source->_values[a]);
            }
        } else {
            _values = NULL;
        }
    }

    size_t _numValues;
    Value **_values;
};

class OperationB1 : public Operation {
    JBALLOC_(OperationB1)

    public:
    virtual size_t size() const        { return sizeof(OperationB1); }
    virtual size_t numBuilders() const { return 1; }
    virtual Builder * builder(uint32_t i=0) const {
        if (i == 0) return _builder;
        return NULL;
    }

    virtual BuilderIterator builders() { return BuilderIterator(allocator(), _builder); }

    protected:
    OperationB1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Builder * b)
        : Operation(MEM_PASSLOC(a), action, ext, parent)
        , _builder(b) {

    }
    OperationB1(Allocator *a, const OperationB1 *source, IRCloner *cloner)
        : Operation(a, source, cloner)
        , _builder(cloner->clonedBuilder(source->_builder)) {

    }

    Builder * _builder;
};

class OperationB1R0V1 : public OperationR0V1 {
    JBALLOC_(OperationB1R0V1)

    public:
    virtual size_t size() const { return sizeof(OperationB1R0V1); }
    virtual size_t numBuilders() const { return 1; }
    virtual Builder * builder(uint32_t i=0) const {
        if (i == 0) return _builder;
        return NULL;
    }
    virtual BuilderIterator builders() { return BuilderIterator(allocator(), _builder); }

    protected:
    OperationB1R0V1(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Builder * b, Value * value)
        : OperationR0V1(MEM_PASSLOC(a), action, ext, parent, value)
        , _builder(b) {

    }
    OperationB1R0V1(Allocator *a, const OperationB1R0V1 *source, IRCloner *cloner)
        : OperationR0V1(a, source, cloner)
        , _builder(cloner->clonedBuilder(source->_builder)) {

    }

    Builder * _builder;
};

class OperationB1R0V2 : public OperationR0V2 {
    JBALLOC_(OperationB1R0V2)

    public:
    virtual size_t size() const        { return sizeof(OperationB1R0V2); }
    virtual size_t numBuilders() const { return 1; }
    virtual Builder * builder(uint32_t i=0) const {
        if (i == 0) return _builder;
        return NULL;
    }
    virtual BuilderIterator builders() { return BuilderIterator(allocator(), _builder); }

    protected:
    OperationB1R0V2(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Builder * b, Value * left, Value * right)
        : OperationR0V2(MEM_PASSLOC(a), action, ext, parent, left, right)
        , _builder(b) {

    }
    OperationB1R0V2(Allocator *a, const OperationB1R0V2 *source, IRCloner *cloner)
        : OperationR0V2(a, source, cloner)
        , _builder(cloner->clonedBuilder(source->_builder)) {

    }

    Builder * _builder;
};


#define IRCLONER_SUPPORT(C,Super) \
protected: \
   C(Allocator *a, const C *source, IRCloner *cloner) : Super(a, source, cloner) { }; \
   virtual Operation *clone(Allocator *mem, IRCloner *cloner) const { return new (mem) C(mem, this, cloner); }


//
// Core operations
//

class Op_AppendBuilder : public OperationB1 {
    JBALLOC_(Op_AppendBuilder)

    friend class CoreExtension;

public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;

protected:
    Op_AppendBuilder(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aAppendBuilder, Builder *b);

    IRCLONER_SUPPORT(Op_AppendBuilder, OperationB1)
};

class Op_MergeDef : public OperationR1V1 {
    JBALLOC_(Op_MergeDef)

    friend class CoreExtension;

public:
    virtual Operation * clone(LOCATION, Builder *b, OperationCloner *cloner) const;

protected:
    Op_MergeDef(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aMergeDef, Value *existingDef, Value *newDef);
    IRCLONER_SUPPORT(Op_MergeDef, OperationR1V1)
};


} // namespace JitBuilder

} // namespace OMR

#endif // defined(OPERATION_INCL)
