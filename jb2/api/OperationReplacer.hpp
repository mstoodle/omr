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

#ifndef OPERATIONREPLACER_INCL
#define OPERATIONREPLACER_INCL

#include <stddef.h>
#include <stdint.h>
#include <set>
#include <string>
#include "Mapper.hpp"
#include "OperationCloner.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Literal;
class Operation;
class Symbol;
class Type;
class Value;

class OperationReplacer : public OperationCloner {
public:
    OperationReplacer(Operation *op);
    ~OperationReplacer();

    void setResultMapper(ValueMapper *m, uint32_t i=0) { if (i < _numResults) _resultMappers[i] = m; }
    void setOperandMapper(ValueMapper *m, uint32_t i=0) { if (i < _numOperands) _operandMappers[i] = m; }
    void setBuilderMapper(BuilderMapper *m, uint32_t i=0) { if (i < _numBuilders) _builderMappers[i] = m; }
    void setLiteralMapper(LiteralMapper *m, uint32_t i=0) { if (i < _numLiterals) _literalMappers[i] = m; }
    void setSymbolMapper(SymbolMapper *m, uint32_t i=0) { if (i < _numSymbols) _symbolMappers[i] = m; }
    void setTypeMapper(TypeMapper *m, uint32_t i=0) { if (i < _numTypes) _typeMappers[i] = m; }

    Operation *operation() const { return _op; }

    ValueMapper *resultMapper(uint32_t i=0) const { if (i < _numResults) return _resultMappers[i]; return NULL; }
    ValueMapper *operandMapper(uint32_t i=0) const { if (i < _numOperands) return _operandMappers[i]; return NULL; }
    BuilderMapper *builderMapper(uint32_t i=0) const { if (i < _numBuilders) return _builderMappers[i]; return NULL; }
    LiteralMapper *literalMapper(uint32_t i=0) const { if (i < _numLiterals) return _literalMappers[i]; return NULL; }
    SymbolMapper *symbolMapper(uint32_t i=0) const { if (i < _numSymbols) return _symbolMappers[i]; return NULL; }
    TypeMapper *typeMapper(uint32_t i=0) const { if (i < _numTypes) return _typeMappers[i]; return NULL; }

    virtual Operation *clone(Builder *b);

protected:
    Operation        * _op;

    ValueMapper     ** _resultMappers;
    ValueMapper     ** _operandMappers;
    BuilderMapper   ** _builderMappers;
    LiteralMapper   ** _literalMappers;
    SymbolMapper    ** _symbolMappers;
    TypeMapper      ** _typeMappers;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(OPERATIONREPLACER_INCL)
