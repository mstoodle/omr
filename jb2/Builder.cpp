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

#include "AllocationCategoryClasses.hpp"
#include "Builder.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "Scope.hpp"
#include "TextLogger.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(Builder, IL);
SUBCLASS_KINDSERVICE_IMPL(Builder, "Builder", ExtensibleIR, Extensible);

Builder::Builder(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind, IR *ir, Scope *scope, String name)
    : ExtensibleIR(a, ext, ir, kind)
    , _id(ir->getBuilderID())
    , _ext(ext)
    , _ir(ir)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, ir->mem())
    , _context(NULL)
    , _scope(scope)
    , _successor(NULL)
    , _operations(NULL, ir->mem())
    , _operationCount(0)
    , _firstOperation(NULL)
    , _lastOperation(NULL)
    , _myLocation(true)
    , _currentLocation(new (ir->mem()) Location(ir->mem(), ir, "", "", 0) )
    , _boundToOperation(NULL)
    , _isTarget(false)
    , _isBound(false)
    , _controlReachesEnd(true) {

    // this constructor is used by subclasses, so defer notifyCreation to subclass's constructor
}

Builder::Builder(Allocator *a, Extension *ext, IR * ir, Scope *scope, String name)
    : ExtensibleIR(a, ext, ir, CLASSKIND(Builder, Extensible))
    , _id(ir->getBuilderID())
    , _ext(ext)
    , _ir(ir)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, ir->mem())
    , _context(NULL)
    , _scope(scope)
    , _successor(NULL)
    , _operations(NULL, ir->mem())
    , _operationCount(0)
    , _firstOperation(NULL)
    , _lastOperation(NULL)
    , _myLocation(true)
    , _currentLocation(new (ir->mem()) Location(ir->mem(), ir, "", "", 0) )
    , _boundToOperation(NULL)
    , _isTarget(false)
    , _isBound(false)
    , _controlReachesEnd(true) {

    notifyCreation(KIND(Extensible));
}

Builder::Builder(Allocator *a, Extension *ext, Builder *parent, Scope *scope, String name)
    : ExtensibleIR(a, ext, parent->ir(), CLASSKIND(Builder, Extensible))
    , _id(parent->ir()->getBuilderID())
    , _ext(ext)
    , _ir(parent->_ir)
    , _name(name)
    , _parent(parent)
    , _children(NULL, parent->ir()->mem())
    , _context(NULL)
    , _scope(scope)
    , _successor(NULL)
    , _operations(NULL, parent->ir()->mem())
    , _operationCount(0)
    , _firstOperation(NULL)
    , _lastOperation(NULL)
    , _myLocation(false)
    , _currentLocation(parent->location())
    , _boundToOperation(NULL)
    , _isTarget(false)
    , _isBound(false)
    , _controlReachesEnd(true) {

    notifyCreation(KIND(Extensible));
    parent->addChild(this);
}

Builder::Builder(Allocator *a, Extension *ext, Builder *parent, Operation *boundToOp, String name)
    : ExtensibleIR(a, ext, parent->ir(), CLASSKIND(Builder, Extensible))
    , _id(parent->ir()->getBuilderID())
    , _ext(ext)
    , _ir(parent->_ir)
    , _name(name)
    , _parent(parent)
    , _children(NULL, parent->ir()->mem())
    , _context(NULL)
    , _scope(parent->scope())
    , _successor(NULL)
    , _operations(NULL, parent->ir()->mem())
    , _operationCount(0)
    , _firstOperation(NULL)
    , _lastOperation(NULL)
    , _myLocation(false)
    , _currentLocation(parent->location())
    , _boundToOperation(boundToOp)
    , _isTarget(false)
    , _isBound(true)
    , _controlReachesEnd(true) {

    notifyCreation(KIND(Extensible));
    parent->addChild(this);
}

Builder::Builder(Allocator *a, const Builder *source, IRCloner *cloner)
    : ExtensibleIR(a, source->_ext, cloner->clonedIR(), CLASSKIND(Builder, Extensible))
    , _id(source->_id)
    , _ext(source->_ext)
    , _ir(cloner->clonedIR())
    , _name(source->_name)
    , _parent(cloner->clonedBuilder(source->_parent))
    , _children(NULL, a)
    , _context(cloner->clonedContext(source->_context))
    , _scope(cloner->clonedScope(source->_scope))
    , _successor(cloner->clonedBuilder(source->_successor))
    , _operations(NULL, a)
    , _operationCount(source->_operationCount)
    , _firstOperation(cloner->clonedOperation(source->_firstOperation))
    , _lastOperation(cloner->clonedOperation(source->_lastOperation))
    , _myLocation(true)
    , _currentLocation(cloner->clonedLocation(source->_currentLocation))
    , _boundToOperation(cloner->clonedOperation(source->_boundToOperation))
    , _isTarget(source->_isTarget)
    , _isBound(source->_isBound)
    , _controlReachesEnd(source->_controlReachesEnd) {

    for (auto it=source->_children.iterator();it.hasItem();it++) {
        Builder *b = it.item();
        _children.push_back(cloner->clonedBuilder(b));
    }

    for (auto it=source->_operations.iterator();it.hasItem();it++) {
        Operation *op = it.item();
        _operations.push_back(cloner->clonedOperation(op));
    }

    // special case: don't notifyCreation because addons are created by ExtensibleIR subclass cloning what the source has
}

Builder *
Builder::cloneBuilder(Allocator *mem, IRCloner *cloner) const {
    return new (mem) Builder(mem, this, cloner);
}

Builder::~Builder() {
    for (Operation *op = firstOperation(); op != NULL; ) {
        Operation *next = op->next();
        delete op;
        op = next;
    }
    if (_myLocation)
        delete _currentLocation;
}

void
Builder::addChild(Builder *child) {
    _children.push_back(child);
}

Builder *
Builder::add(Operation *op) {
    _operations.push_back(op);
    op->setNext(NULL);
    if (_firstOperation == NULL) {
        _firstOperation = op;
	op->setPrev(NULL);
    }
    else {
        _lastOperation->setNext(op);
	op->setPrev(_lastOperation);
    }
    _lastOperation = op;
    _operationCount++;
    return this;
}

String
Builder::to_string() const {
    return String(_ir->mem(), "B").append(String::to_string(_ir->mem(), _id));
}

void
Builder::logProperties(TextLogger & lgr) const {
    if (parent())
        lgr.indent() << "[ parent " << parent() << " ]" << lgr.endl();
    else
        lgr.indent() << "[ parent NULL ]" << lgr.endl();

    lgr.indent() << "[ scope " << scope() << " ]" << lgr.endl();

    if (numChildren() > 0) {
        lgr.indent() << "[ children" << lgr.endl();
        lgr.indentIn();
        for (BuilderListIterator it = childrenIterator(); it.hasItem(); it++) {
             Builder *child = it.item();
             lgr.indent() << "[ " << child << " ]" << lgr.endl();
         }
         lgr.indentOut();
         lgr.indent() << "]" << lgr.endl();
    }

    if (isBound())
        lgr.indent() << "[ bound " << boundToOperation() << " ]" << lgr.endl();
    else
        lgr.indent() << "[ notBound ]" << lgr.endl();

    if (isTarget())
        lgr.indent() << "[ isTarget ]" << lgr.endl();
    else
        lgr.indent() << "[ notTarget ]" << lgr.endl();

    // deprecate
    if (controlReachesEnd())
        lgr.indent() << "[ controlReachesEnd ]" << lgr.endl();
    else
        lgr.indent() << "[ notControlReachesEnd ]" << lgr.endl();
}

void
Builder::logPrefix(TextLogger & lgr) const {
    lgr.indent() << "[ " << logName() << " " << this;
    if (name().length() > 0)
        lgr << " \"" << name() << "\"";
    lgr << lgr.endl();
    lgr.indentIn();

    logProperties(lgr);

    lgr.indent() << "[ operations" << lgr.endl();
    lgr.indentIn();
}

void
Builder::logSuffix(TextLogger & lgr) const {
    lgr.indentOut();
    lgr.indent() << "]" << lgr.endl(); // operations
    lgr.indentOut();
    lgr.indent() << "]" << lgr.endl(); // builder
}

} // namespace JB2
} // namespace OMR
