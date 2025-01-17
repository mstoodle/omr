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

#include "Compilation.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "OperationReplacer.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(OperationReplacer, Compilation)

OperationReplacer::OperationReplacer(Allocator *a, Operation *op)
    : OperationCloner(a, op)
    , _resultMappers(NULL)
    , _operandMappers(NULL)
    , _builderMappers(NULL)
    , _literalMappers(NULL)
    , _symbolMappers(NULL)
    , _typeMappers(NULL) {

    if (_numResults > 0) {
        _resultMappers = a->allocate<ValueMapper *>(_numResults);
        for (auto i=0;i < _numResults;i++)
            _resultMappers[i] = NULL;
    }

    if (_numOperands > 0) {
        _operandMappers = a->allocate<ValueMapper *>(_numOperands);
        for (auto i=0;i < _numOperands;i++)
            _operandMappers[i] = NULL;
    }

    if (_numBuilders > 0) {
        _builderMappers = a->allocate<BuilderMapper *>(_numBuilders);
        for (auto i=0;i < _numBuilders;i++)
            _builderMappers[i] = NULL;
    }

    if (_numLiterals > 0) {
        _literalMappers = a->allocate<LiteralMapper *>(_numLiterals);
        for (auto i=0;i < _numLiterals;i++)
            _literalMappers[i] = NULL;
    }

    if (_numSymbols > 0) {
        _symbolMappers = a->allocate<SymbolMapper *>(_numSymbols);
        for (auto i=0;i < _numSymbols;i++)
            _symbolMappers[i] = NULL;
    }

    if (_numTypes > 0) {
        _typeMappers = a->allocate<TypeMapper *>(_numTypes);
        for (auto i=0;i < _numTypes;i++)
            _typeMappers[i] = NULL;
    }
}

OperationReplacer::~OperationReplacer() {
    Allocator *mem = allocator();
    if (_resultMappers)
        mem->deallocate(_resultMappers);
    if (_operandMappers)
        mem->deallocate(_operandMappers);
    if (_builderMappers)
        mem->deallocate(_builderMappers);
    if (_literalMappers)
        mem->deallocate(_literalMappers);
    if (_symbolMappers)
        mem->deallocate(_symbolMappers);
    if (_typeMappers)
        mem->deallocate(_typeMappers);
}

Operation *
OperationReplacer::clone(Builder *b) {
    for (uint32_t i=0;i < _numOperands;i++)
        changeOperand(operandMapper(i)->next(), i);
    for (uint32_t i=0;i < _numTypes;i++)
        changeType(typeMapper(i)->next(), i);
    for (uint32_t i=0;i < _numLiterals;i++)
        changeLiteral(literalMapper(i)->next(), i);
    for (uint32_t i=0;i < _numSymbols;i++)
        changeSymbol(symbolMapper(i)->next(), i);
    for (uint32_t i=0;i < _numBuilders;i++)
        changeBuilder(builderMapper(i)->next(), i);

    Operation *clonedOp = OperationCloner::clone(b);

    for (uint32_t i=0;i < _numResults;i++)
        resultMapper(i)->add( result(i) );

    return clonedOp;
}

} // namespace JB2
} // namespace OMR
