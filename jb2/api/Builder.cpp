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
#include "Compilation.hpp"
#include "Context.hpp"
#include "JB1MethodBuilder.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "TextLogger.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Builder, IL)

Builder::Builder(Allocator *a, Compilation * comp, Context *context, String name)
    : Allocatable(a)
    , _id(comp->getBuilderID())
    , _comp(comp)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, comp->mem())
    , _context(context)
    , _successor(NULL)
    , _operations(NULL, comp->mem())
    , _operationCount(0)
    , _firstOperation(NULL)
    , _lastOperation(NULL)
    , _myLocation(true)
    , _currentLocation(new (comp->mem()) Location(comp->mem(), comp, "", "", 0) )
    , _boundToOperation(NULL)
    , _isTarget(false)
    , _isBound(false)
    , _controlReachesEnd(true) {
}

Builder::Builder(Allocator *a, Builder *parent, Context *context, String name)
    : Allocatable(a)
    , _id(parent->_comp->getBuilderID())
    , _comp(parent->_comp)
    , _name(name)
    , _parent(parent)
    , _children(NULL, parent->comp()->mem())
    , _context(context)
    , _successor(NULL)
    , _operations(NULL, parent->comp()->mem())
    , _operationCount(0)
    , _firstOperation(NULL)
    , _lastOperation(NULL)
    , _myLocation(false)
    , _currentLocation(parent->location())
    , _boundToOperation(NULL)
    , _isTarget(false)
    , _isBound(false)
    , _controlReachesEnd(true) {
    parent->addChild(this);
}

Builder::Builder(Allocator *a, Builder *parent, Operation *boundToOp, String name)
    : Allocatable(a)
    , _id(parent->_comp->getBuilderID())
    , _comp(parent->_comp)
    , _name(name)
    , _parent(parent)
    , _children(NULL, parent->comp()->mem())
    , _context(parent->context())
    , _successor(NULL)
    , _operations(NULL, parent->comp()->mem())
    , _operationCount(0)
    , _firstOperation(NULL)
    , _lastOperation(NULL)
    , _myLocation(false)
    , _currentLocation(parent->location())
    , _boundToOperation(boundToOp)
    , _isTarget(false)
    , _isBound(true)
    , _controlReachesEnd(true) {
    parent->addChild(this);
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

Builder *
Builder::create(Builder *parent, Context *context, String name) {
    Allocator *mem = parent->allocator();
    return new (mem) Builder(mem, parent, context, name);
}

Builder *
Builder::create(Compilation *comp, Context *context, String name) {
    Allocator *mem = comp->mem();
    return new (mem) Builder(mem, comp, context, name);
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

void
Builder::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->createBuilder(this);
}

void
Builder::jbgenSuccessors(JB1MethodBuilder *j1mb) const {
}

void
Builder::logProperties(TextLogger & lgr) const {
    if (parent())
        lgr.indent() << "[ parent " << parent() << " ]" << lgr.endl();
    else
        lgr.indent() << "[ parent NULL ]" << lgr.endl();

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

} // namespace JitBuilder
} // namespace OMR
