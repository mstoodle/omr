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

#include "Builder.hpp"
#include "Compilation.hpp"
#include "CreateLoc.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "OperationReplacer.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(OperationCloner, Compilation)

OperationCloner::OperationCloner(Allocator *a, Operation *op)
    : Allocatable(a)
    , _op(op)
    , _numResults(op->numResults())
    , _results(NULL)
    , _numOperands(op->numOperands())
    , _operands(NULL)
    , _numTypes(op->numTypes())
    , _types(NULL)
    , _numLiterals(op->numLiterals())
    , _literals(NULL)
    , _numSymbols(op->numSymbols())
    , _symbols(NULL)
    , _numBuilders(op->numBuilders())
    , _builders(NULL) {
    init();
    reset();
}

OperationCloner::~OperationCloner() {
    Allocator *mem = allocator();

    if (_results)
        mem->deallocate(_results);

    if (_operands)
        mem->deallocate(_operands);

    if (_types)
        mem->deallocate(_types);

    if (_literals)
        mem->deallocate(_literals);

    if (_symbols)
        mem->deallocate(_symbols);

    if (_builders)
        mem->deallocate(_builders);
}

void
OperationCloner::init() {
    Allocator *mem = allocator();

    if (_numResults > 0)
        _results = mem->allocate<Value *>(_numResults);

    if (_numOperands > 0)
        _operands = mem->allocate<Value *>(_numOperands);

    if (_numTypes > 0)
        _types = mem->allocate<const Type *>(_numTypes);

    if (_numLiterals > 0)
        _literals = mem->allocate<Literal *>(_numLiterals);

    if (_numSymbols > 0)
        _symbols = mem->allocate<Symbol *>(_numSymbols);

    if (_numBuilders > 0)
        _builders = mem->allocate<Builder *>(_numBuilders);
}

void
OperationCloner::reset() {
    for (uint32_t i=0;i < _numResults;i++)
        _results[i] = _op->result(i);
    for (uint32_t i=0;i < _numOperands;i++)
        _operands[i] = _op->operand(i);
    for (uint32_t i=0;i < _numTypes;i++)
        _types[i] = _op->type(i);
    for (uint32_t i=0;i < _numLiterals;i++)
        _literals[i] = _op->literal(i);
    for (uint32_t i=0;i < _numSymbols;i++)
        _symbols[i] = _op->symbol(i);
    for (uint32_t i=0;i < _numBuilders;i++)
        _builders[i] = _op->builder(i);
}

void
OperationCloner::createResult(Builder *b, uint32_t i) {
    changeResult( Value::create(b, _op->result(i)->type()) );
}

Operation *
OperationCloner::clone(Builder *b) {
    return _op->clone(LOC, b, this);
}

} // namespace JB2
} // namespace OMR
