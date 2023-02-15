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

#include <cassert>
#include "Func/Func.hpp"
#include "OperationSimulator.hpp"
#include "SimDictionary.hpp"
#include "SimulatorFunction.hpp"

namespace OMR {
namespace JitBuilder {
namespace Sim {

OperationSimulator::OperationSimulator(Allocator *a, Operation *op)
    : OperationCloner(a, op)
    , _parentCompiler(op->parent()->comp()->compiler())
    , _simCompiler(NULL)
    , _body(NULL)
    , _literalOperandValues(NULL)
    , _literalResultValues(NULL)
    , _literalSymbolValues(NULL) {

    Allocator *mem = op->parent()->comp()->mem();
    if (op->numLiterals() > 0)
        _literalOperandValues = mem->allocate<Literal *>(op->numLiterals());
    if (op->numResults() > 0)
        _literalResultValues = mem->allocate<Literal *>(op->numResults());
    if (op->numSymbols() > 0)
        _literalSymbolValues = mem->allocate<Literal *>(op->numSymbols());
}

OperationSimulator::~OperationSimulator() {
    Allocator *mem = _op->parent()->comp()->mem();
    if (_body)
        delete _body;
    if (_literalSymbolValues)
        mem->deallocate(_literalSymbolValues);
    if (_literalResultValues)
        mem->deallocate(_literalResultValues);
    if (_literalOperandValues)
        mem->deallocate(_literalOperandValues);
}

OperationSimulator *
OperationSimulator::specifyOperand(Literal *lit, uint32_t i) {
    if (i < _numOperands) {
        assert(lit->type() == _op->operand(i)->type());
        _literalOperandValues[i] = lit;
    }
    return this;
}

OperationSimulator *
OperationSimulator::specifySymbol(Literal *lit, uint32_t i) {
    if (i < _numSymbols) {
        assert(lit->type() == _op->symbol(i)->type());
        _literalSymbolValues[i] = lit;
    }
    return this;
}

bool
OperationSimulator::simulate() {
    Compiler compiler(_parentCompiler, "OperationSimulator");
    _simCompiler = &compiler;

    Compilation *op_comp = _op->parent()->comp();

    SimulatorFunction simFunc(_simCompiler);

    LiteralDictionary *origLitDict = op_comp->litdict();
    LiteralDictionary simLitDict(_simCompiler, "sim_litdict", origLitDict);

    SymbolDictionary *origSymDict = op_comp->symdict();
    SymbolDictionary simSymDict(_simCompiler, "sim_symdict", origSymDict);

    TypeDictionary *origTypeDict = op_comp->typedict();
    SimDictionary simTypeDict(_simCompiler, "sim_typedict", origTypeDict);

    Config *cfg = op_comp->config();
    Config simCfg(cfg);

    Func::FunctionCompilation comp(_simCompiler, _simFunc, &simLitDict, &simSymDict, &simTypeDict, &simCfg);

}

} // namespace Sim
} // namespace JitBuilder
} // namespace OMR
