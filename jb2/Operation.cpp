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
#include "IR.hpp"
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
namespace JB2 {


INIT_JBALLOC(Operation)
SUBCLASS_KINDSERVICE_IMPL(Operation,"Operation",ExtensibleIR, Extensible)

Operation::Operation(MEM_LOCATION(a), ActionID action, Extension *ext, Builder * parent, Operation *next, Operation *prev)
    : ExtensibleIR(a, ext, parent->ir(), CLASSKIND(Operation, Extensible))
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
    : ExtensibleIR(a, source, cloner)
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

INIT_JBALLOC_REUSECAT(OperationR1T1V1, Operation)

OperationR1T1V1::~OperationR1T1V1() {
}

void
OperationR1T1V1::log(TextLogger & lgr) const {
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


} // namespace JB2
} // namespace OMR

