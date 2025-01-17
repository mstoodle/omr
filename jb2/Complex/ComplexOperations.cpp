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
#include "Compiler.hpp"
#include "ComplexExtension.hpp"
#include "ComplexOperations.hpp"
#include "Builder.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "TextWriter.hpp"
#include "Value.hpp"

using namespace OMR::JB2;

namespace OMR {
namespace JB2 {
namespace Complex {

//
// Real
//
Op_Real::Op_Real(LOCATION, Extension *ext, Builder * parent, ActionID aReal, Value *result, Value *value)
    : OperationR1V1(PASSLOC, aReal, ext, parent, result, value) {
}

Operation *
Op_Real::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Real(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

void
Op_Real::jbgen(JB1MethodBuilder *j1mb) const {
    CompilationException e(LOC, this->_ext->compiler(), this->_ext->compiler()->CompileFail_TypeMustBeReduced);
    e.setMessage(String("Op_Real::jbgen: ComplexTypes must be reduced before jb1codegen"))
     .appendMessage(String("    Operation B").append(String::to_string(this->parent()->id())).append(String("!op")).append(String::to_string(this->id())));
    throw e;
}


//
// Imag
//
Op_Imag::Op_Imag(LOCATION, Extension *ext, Builder * parent, ActionID aImag, Value *result, Value *value)
    : OperationR1V1(PASSLOC, aImag, ext, parent, result, value) {
}

Operation *
Op_Imag::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Imag(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

void
Op_Imag::jbgen(JB1MethodBuilder *j1mb) const {
    CompilationException e(LOC, this->_ext->compiler(), this->_ext->compiler()->CompileFail_TypeMustBeReduced);
    e.setMessage(String("Op_Imag::jbgen: ComplexTypes must be reduced before jb1codegen"))
     .appendMessage(String("    Operation B").append(String::to_string(this->parent()->id())).append(String("!op")).append(String::to_string(this->id())));
    throw e;
}


//
// Conjugate
//
Op_Conjugate::Op_Conjugate(LOCATION, Extension *ext, Builder * parent, ActionID aConjugate, Value *result, Value *value)
    : OperationR1V1(PASSLOC, aConjugate, ext, parent, result, value) {
}

Operation *
Op_Conjugate::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Conjugate(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

void
Op_Conjugate::jbgen(JB1MethodBuilder *j1mb) const {
    CompilationException e(LOC, this->_ext->compiler(), this->_ext->compiler()->CompileFail_TypeMustBeReduced);
    e.setMessage(String("Op_Conjugate::jbgen: ComplexTypes must be reduced before jb1codegen"))
     .appendMessage(String("    Operation B").append(String::to_string(this->parent()->id())).append(String("!op")).append(String::to_string(this->id())));
    throw e;
}


//
// Magnitude
//
Op_Magnitude::Op_Magnitude(LOCATION, Extension *ext, Builder * parent, ActionID aMagnitude, Value *result, Value *value)
    : OperationR1V1(PASSLOC, aMagnitude, ext, parent, result, value) {
}

Operation *
Op_Magnitude::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Magnitude(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->operand(0));
}

void
Op_Magnitude::jbgen(JB1MethodBuilder *j1mb) const {
    CompilationException e(LOC, this->_ext->compiler(), this->_ext->compiler()->CompileFail_TypeMustBeReduced);
    e.setMessage(String("Op_Magnitude::jbgen: ComplexTypes must be reduced before jb1codegen"))
     .appendMessage(String("    Operation B").append(String::to_string(this->parent()->id())).append(String("!op")).append(String::to_string(this->id())));
    throw e;
}


} // namespace Complex
} // namespace JB2
} // namespace OMR
