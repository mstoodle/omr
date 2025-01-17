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

#include "common.hpp"
#include "ExtensibleIR.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 { 

class Context;
class Extension;
class FunctionBuilder;
class IR;
class IRCloner;
class Location;
class Operation;
class OperationBuilder;
class OperationCloner;
class Scope;
class Type;
class Value;
class TextLogger;
class Transformer;
class TypeDictionary;

class Builder : public ExtensibleIR {
    JBALLOC_(Builder);

    friend class CoreExtension;
    friend class Extension;
    friend class IRCloner;
    friend class Operation;
    friend class OperationBuilder;
    friend class Transformer;

public:
    int64_t id() const                                  { return _id; }
    const String & name() const                         { return _name; }
    IR *ir() const                                      { return _ir; }
    Extension *ext() const                              { return _ext; }

    TypeDictionary * dict() const;
    Builder * parent() const                            { return _parent; }

    Scope * scope() const                               { return _scope; }

    int32_t numChildren() const                         { return _children.length(); }
    BuilderListIterator childrenIterator() const        { return _children.iterator(); }

    int32_t numOperations() const                       { return _operationCount; }
    Operation *firstOperation() const                   { return _firstOperation; }
    Operation *lastOperation() const                    { return _lastOperation; }

    bool isBound() const                                { return _isBound; }
    Operation * boundToOperation() const                { assert(_isBound); return _boundToOperation; }

    bool isTarget() const                               { return _isTarget; }
    Builder * setTarget(bool v=true)                    { _isTarget = v; return this; }

    bool controlReachesEnd() const                      { return _controlReachesEnd; }
    Builder * setControlReachesEnd(bool v=true)         { _controlReachesEnd = v; return this; }

    Location *location() const {
        return _currentLocation;
    }
    void setLocation(Location * loc) {
        _myLocation = false;
        _currentLocation = loc;
    }

    virtual String to_string() const;
    virtual String logName() const { return String(allocator(), "Builder"); }
    virtual void logProperties(TextLogger & log) const;
    virtual void logPrefix(TextLogger & log) const;
    virtual void logSuffix(TextLogger & log) const;

protected:
    DYNAMIC_ALLOC_ONLY(Builder, Extension *ext, KINDTYPE(Extensible) kind, IR *ir, Scope *scope=NULL, String name="");
    DYNAMIC_ALLOC_ONLY(Builder, Extension *ext, IR *ir, Scope *scope=NULL, String name="");
    DYNAMIC_ALLOC_ONLY(Builder, Extension *ext, Builder *parent, Scope *scope=NULL, String name="");
    DYNAMIC_ALLOC_ONLY(Builder, Extension *ext, Builder *parent, Operation *boundToOp, String name="");

    Builder(Allocator *a, const Builder *source, IRCloner *cloner);

    virtual ExtensibleIR *clone(Allocator *mem, IRCloner *cloner) const { return cloneBuilder(mem, cloner); }
    virtual Builder *cloneBuilder(Allocator *mem, IRCloner *cloner) const;

    void setParent(Builder *parent);
    void addChild(Builder *child);
    Builder * add(Operation * op);
    Builder * setBound(Operation *op) { _isBound = true; _boundToOperation = op; return this; }

    BuilderID            _id;
    Extension          * _ext;
    IR                 * _ir;
    String               _name;
    Builder            * _parent;
    List<Builder *>      _children;
    Context            * _context;
    Scope              * _scope;
    Builder            * _successor;
    List<Operation *>    _operations;
    int32_t              _operationCount;
    Operation          * _firstOperation;
    Operation          * _lastOperation;
    bool                 _myLocation;
    Location           * _currentLocation;
    Operation          * _boundToOperation;
    bool                 _isTarget;
    bool                 _isBound;
    bool                 _controlReachesEnd;

    SUBCLASS_KINDSERVICE_DECL(Extensible, Builder);
};

} // namespace JB2
} // namespace OMR

#endif // defined(BUILDER_INCL)
