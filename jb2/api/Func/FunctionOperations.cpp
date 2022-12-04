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

using namespace OMR::JitBuilder;

namespace OMR {
namespace JitBuilder {
namespace Func {

//
// Load
//
Op_Load::Op_Load(LOCATION, Extension *ext, Builder * parent, ActionID aLoad, Value *result, Symbol *symbol)
    : OperationR1S1(PASSLOC, aLoad, ext, parent, result, symbol) {
}

Operation *
Op_Load::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Load(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->symbol());
}

void
Op_Load::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Load(location(), this->parent(), this->_result, this->_symbol);
}


//
// Store
//
Op_Store::Op_Store(LOCATION, Extension *ext, Builder * parent, ActionID aStore, Symbol *symbol, Value *value)
    : OperationR0S1V1(PASSLOC, aStore, ext, parent, symbol, value) {
    assert(value);
}

Operation *
Op_Store::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Store(PASSLOC, this->_ext, b, this->action(), cloner->symbol(), cloner->operand());
}

void
Op_Store::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Store(location(), this->parent(), this->_symbol, this->_value);
}


//
// Call
//

Op_Call::Op_Call(LOCATION, Extension *ext, Builder * parent, ActionID aCall, Value *result, FunctionSymbol *target, std::va_list & args)
    : OperationR1S1VN(PASSLOC, aCall, ext, parent, result, target, target->functionType()->numParms(), args) {

}

Operation *
Op_Call::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Call(PASSLOC, this->_ext, b, this->action(), cloner);
}

void
Op_Call::write(TextWriter & w) const {
    if (_result)
        w << this->_result << " = ";
    w << name() << " " << this->_symbol;
    for (auto a=0;a < _values.size(); a++) {
        w << " " << this->_values[a];
    }
    w << w.endl();
}

void
Op_Call::jbgen(JB1MethodBuilder *j1mb) const {
    FunctionSymbol *funcSym = symbol()->refine<FunctionSymbol>();
    const FunctionType *funcType = funcSym->functionType();
    //j1mb->DefineFunction(funcSym->name(), funcSym->fileName(), funcSym->lineNumber(), funcSym->entryPoint(), funcType->returnType(), funcType->numParms(), funcType->parmTypes());
    j1mb->Call(location(), parent(), result(), funcSym->name(), _values);
}


//
// CallVoid
//

Op_CallVoid::Op_CallVoid(LOCATION, Extension *ext, Builder * parent, ActionID aCall, FunctionSymbol *target, std::va_list & args)
    : OperationR0S1VN(PASSLOC, aCall, ext, parent, target, target->functionType()->numParms(), args) {

}

Operation *
Op_CallVoid::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_CallVoid(PASSLOC, this->_ext, b, this->action(), cloner);
}

void
Op_CallVoid::write(TextWriter & w) const {
    w << name() << " " << this->_symbol;
    for (auto a=0;a < _values.size(); a++) {
        w << " " << this->_values[a];
    }
    w << w.endl();
}

void
Op_CallVoid::jbgen(JB1MethodBuilder *j1mb) const {
    FunctionSymbol *funcSym = symbol()->refine<FunctionSymbol>();
    const FunctionType *funcType = funcSym->functionType();
    //j1mb->DefineFunction(funcSym->name(), funcSym->fileName(), funcSym->lineNumber(), funcSym->entryPoint(), funcType->returnType(), funcType->numParms(), funcType->parmTypes());
    j1mb->Call(location(), parent(), funcSym->name(), _values);
}


//
// ReturnVoid
//
Op_ReturnVoid::Op_ReturnVoid(LOCATION, Extension *ext, Builder * parent, ActionID aReturnVoid)
    : Operation(PASSLOC, aReturnVoid, ext, parent) {
    parent->setControlReachesEnd(false);
}

Operation *
Op_ReturnVoid::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_ReturnVoid(PASSLOC, this->_ext, b, this->action());
}

void
Op_ReturnVoid::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Return(location(), parent());
}


//
// Return
//
Op_Return::Op_Return(LOCATION, Extension *ext, Builder * parent, ActionID aReturn, Value * v)
    : OperationR0V1(PASSLOC, aReturn, ext, parent, v) {
    parent->setControlReachesEnd(false);
}

Operation *
Op_Return::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Return(PASSLOC, this->_ext, b, this->action(), cloner->operand());
}

void
Op_Return::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->Return(location(), parent(), operand());
}


} // namespace Func
} // namespace JitBuilder
} // namespace OMR
