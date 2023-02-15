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

#ifndef OPERATIONSIMULATOR_INCL
#define OPERATIONSIMULATOR_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JitBuilder {
namespace Sim {

// OperationSimulator is designed to simulate the execution of a single Operation, but that
// may include the execution of other Operations inside Builder objects that are bound to
// the target Operation or bound to an Operation whose parent Builder is bound to the target
// Operation (transitively). The OperationSimulator object uses a secondary Compiler to compile
// the target Operation (along with bound Builders) into a function that is then called by the
// OperationSimulator to compute the results and outbound control flow.
//
// The OperationSimulator accepts a set of Literals to replace the operand Values and Symbols.
// After simulation, the OperationSimulator holds a set of return values that are Literals and
// also an outbound control flow direction (a BuilderID). If the operation completed normally,
// the outbound BuilderID is the ID of the Operation's parent Builder. Otherwise, it will be
// the BuilderID to which control was directed (in the case of a conditional branch, for example).
// The OperationSimulator can be configured to fully execute the Operation (in which case it
// will only return the Operation's parent Builder ID even if other Operations in other Builders
// need to be simulated), or to partially execute the Operation (in which case only the part
// of the Operation will be simulated up until control needs to transfer to another Operation
// even if this Operation is not yet done).
//

class OperationSimulator : public OperationCloner {
    JBALLOC_(OperationSimulator)

public:
    DYNAMIC_ALLOC_ONLY(OperationSimulator, Operation *op);

    bool simulate();

    OperationSimulator *specifyOperand(Literal *lit, uint32_t i=0);
    OperationSimulator *specifySymbol(Literal *lit, uint32_t i=0);

    // This class doesn't really "clone" an operation it just reuses code
    virtual Operation *clone(Builder *b) { return NULL; }

    uint32_t numResults() const {
        return _numResults;
    }
    Value *result(uint32_t i=0) const {
        if (i < _numResults) return _results[i];
        return NULL;
    }


protected:
    Compiler * _parentCompiler;
    Compiler * _simCompiler;
    CompiledBody *_body;

    Literal **_literalOperandValues;
    Literal **_literalSymbolValues;
    Literal **_literalResultValues;
};

} // namespace Sim
} // namespace JitBuilder
} // namespace OMR

#endif // defined(OPERATIONSIMULATOR_INCL)
