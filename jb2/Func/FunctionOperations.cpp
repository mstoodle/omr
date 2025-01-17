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
#include "JBCore.hpp"
#include "Func/FunctionOperations.hpp"
#include "Func/FunctionSymbols.hpp"
#include "Func/FunctionType.hpp"

using namespace OMR::JB2;

namespace OMR {
namespace JB2 {
namespace Func {

//
// Load
//
INIT_JBALLOC_REUSECAT(Op_Load, Operation)

Op_Load::Op_Load(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aLoad, Value *result, Symbol *symbol)
    : OperationR1S1(MEM_PASSLOC(a), aLoad, ext, parent, result, symbol) {
}

Op_Load::~Op_Load() {

}

Operation *
Op_Load::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Load(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->symbol());
}


//
// Store
//
INIT_JBALLOC_REUSECAT(Op_Store, Operation)

Op_Store::Op_Store(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aStore, Symbol *symbol, Value *value)
    : OperationR0S1V1(MEM_PASSLOC(a), aStore, ext, parent, symbol, value) {
    assert(value);
}

Op_Store::~Op_Store() {

}

Operation *
Op_Store::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Store(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->symbol(), cloner->operand());
}


//
// Call
//

INIT_JBALLOC_REUSECAT(Op_Call, Operation)

Op_Call::Op_Call(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCall, Value *result, FunctionSymbol *target, std::va_list & args)
    : OperationR1S1VN(MEM_PASSLOC(a), aCall, ext, parent, result, target, target->functionType()->numParms(), args) {

}

Op_Call::~Op_Call() {

}

Operation *
Op_Call::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Call(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner);
}

void
Op_Call::log(TextLogger & lgr) const {
    if (_result)
        lgr << this->_result << " = ";
    lgr << name() << " " << this->_symbol;
    for (auto a=0;a < _numValues; a++) {
        lgr << " " << this->_values[a];
    }
    lgr << lgr.endl();
}


//
// CallVoid
//

INIT_JBALLOC_REUSECAT(Op_CallVoid, Operation)

Op_CallVoid::Op_CallVoid(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCall, FunctionSymbol *target, std::va_list & args)
    : OperationR0S1VN(MEM_PASSLOC(a), aCall, ext, parent, target, target->functionType()->numParms(), args) {

}

Op_CallVoid::~Op_CallVoid() {

}

Operation *
Op_CallVoid::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_CallVoid(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner);
}

void
Op_CallVoid::log(TextLogger & lgr) const {
    lgr << name() << " " << this->_symbol;
    for (auto a=0;a < this->_numValues; a++) {
        lgr << " " << this->_values[a];
    }
    lgr << lgr.endl();
}


//
// ReturnVoid
//
INIT_JBALLOC_REUSECAT(Op_ReturnVoid, Operation)

Op_ReturnVoid::Op_ReturnVoid(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aReturnVoid)
    : Operation(MEM_PASSLOC(a), aReturnVoid, ext, parent) {
    parent->setControlReachesEnd(false);
}

Op_ReturnVoid::~Op_ReturnVoid() {

}

Operation *
Op_ReturnVoid::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_ReturnVoid(MEM_PASSLOC(mem), this->_ext, b, this->action());
}


//
// Return
//
INIT_JBALLOC_REUSECAT(Op_Return, Operation)

Op_Return::Op_Return(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aReturn, Value * v)
    : OperationR0V1(MEM_PASSLOC(a), aReturn, ext, parent, v) {
    parent->setControlReachesEnd(false);
}

Op_Return::~Op_Return() {

}

Operation *
Op_Return::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Return(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->operand());
}


} // namespace Func
} // namespace JB2
} // namespace OMR
