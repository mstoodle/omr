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

#ifndef BUILDER_INCL
#define BUILDER_INCL

#include <cassert>
#include <stdint.h>
#include <string>
#include <vector>
#include "IDs.hpp"
#include "Iterator.hpp"

namespace OMR {
namespace JitBuilder { 

class Compilation;
class Context;
class FunctionBuilder;
class JB1MethodBuilder;
class Location;
class Operation;
class OperationBuilder;
class OperationCloner;
class Type;
class Value;
class TextWriter;
class Transformer;
class TypeDictionary;

typedef std::vector<Operation *> OperationVector;
typedef OperationVector::iterator OperationIterator;

class Builder
    {
    friend class Extension;
    friend class OperationBuilder;
    friend class Transformer;

public:
    virtual ~Builder();

    Compilation *comp() const { return _comp; }

    static Builder * create(Builder *parent, Context *context=NULL, std::string name="");
    static Builder * create(Compilation *comp, Context *context=NULL, std::string name="");

    #if 0
    Operation * appendClone(Operation *op);
    Operation * appendClone(Operation *op, OperationCloner *cloner);

    Operation * Append(OperationBuilder *opBuilder);
    Value * Append(OperationBuilder *opBuilder, Literal *l);
    Value * Append(OperationBuilder *opBuilder, Value *v);
    Value * Append(OperationBuilder *opBuilder, Value *left, Value *right);

    Value * CoercePointer(Type * t, Value * v);

    void AppendBuilder(Builder * b);
    void IfThenElse(Builder * thenB, Value * cond);
    void IfThenElse(Builder * thenB, Builder * elseB, Value * cond);
    void ForLoopUp(std::string loopVar, Builder * body, Value * initial, Value * end, Value * bump);
    void ForLoopUp(LocalSymbol *loopSym, Builder * body, Value * initial, Value * end, Value * bump);
    void ForLoop(bool countsUp, std::string loopVar, Builder * body, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump);
    void ForLoop(bool countsUp, LocalSymbol *loopSym, Builder * body, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump);
    void Switch(Value * selector, Builder * defaultBuilder, int numCases, Case ** cases);

    Value * CreateLocalArray(int32_t numElements, Type *elementType);
    Value * CreateLocalStruct(Type *elementType);
    #endif

    Location * SourceLocation();
    Location * SourceLocation(std::string lineNumber);
    Location * SourceLocation(std::string lineNumber, int32_t bcIndex);

    int64_t id() const                                  { return _id; }
    std::string name() const                            { return _name; }
    virtual size_t size() const                         { return sizeof(Builder); }

    TypeDictionary * dict() const;
    Builder * parent() const                            { return _parent; }

    Context * context() const                           { return _context; }

    int32_t numChildren() const                         { return _children.size(); }
    BuilderIterator ChildrenBegin() const               { return BuilderIterator(_children); }
    BuilderIterator ChildrenEnd() const                 { return BuilderIterator(); }

    int32_t numOperations() const                       { return _operations.size(); }
    OperationVector & operations()                      { return _operations; }
    OperationIterator OperationsBegin()                 { return _operations.begin(); }
    OperationIterator OperationsEnd()                   { return _operations.end(); }

    bool isBound() const                                { return _isBound; }
    Operation * boundToOperation() const                { assert(_isBound); return _boundToOperation; }
    Builder * setBound(Operation *op)                   { _isBound = true; _boundToOperation = op; return this; }

    bool isTarget() const                               { return _isTarget; }
    Builder * setTarget(bool v=true);

    bool controlReachesEnd() const                      { return _controlReachesEnd; }
    Builder * setControlReachesEnd(bool v=true)         { _controlReachesEnd = v; return this; }

    Location *location() const {
        return _currentLocation;
    }
    void setLocation(Location * loc) {
        _currentLocation = loc;
    }

    virtual std::string to_string() const {
        return std::string("B").append(std::to_string(_id));
    }

    virtual std::string logName() const { return "Builder"; }
    virtual void writeProperties(TextWriter & w) const;
    virtual void writePrefix(TextWriter & w) const;
    virtual void writeSuffix(TextWriter & w) const;

    virtual void jbgen(JB1MethodBuilder *j1mb) const;
    virtual void jbgenSuccessors(JB1MethodBuilder *j1mb) const;

    protected:
    Builder(Compilation *comp, Context *context=NULL, std::string name="");
    Builder(Builder *parent, Context *context=NULL, std::string name="");
    Builder(Builder *parent, Operation *boundToOp, std::string name="");

    void setParent(Builder *parent);
    void addChild(Builder *child);
    Builder * add(Operation * op);

    BuilderID              _id;
    Compilation          * _comp;
    std::string            _name;
    Builder              * _parent;
    std::vector<Builder *> _children;
    Context              * _context;
    Builder              * _successor;
    OperationVector        _operations;
    Location             * _currentLocation;
    Operation            * _boundToOperation;
    bool                   _isTarget;
    bool                   _isBound;
    bool                   _controlReachesEnd;
    };

} // namespace JitBuilder
} // namespace OMR

#endif // defined(BUILDER_INCL)
