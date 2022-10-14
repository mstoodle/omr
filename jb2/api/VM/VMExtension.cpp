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

#include "Base/BaseExtension.hpp"
#include "Base/FunctionCompilation.hpp"
#include "BytecodeBuilder.hpp"
#include "Compiler.hpp"
#include "VMExtension.hpp"

namespace OMR {
namespace JitBuilder {
namespace VM {

const SemanticVersion VMExtension::version(VMEXT_MAJOR,VMEXT_MINOR,VMEXT_PATCH);
const SemanticVersion VMExtension::requiredBaseVersion(REQUIRED_BASEEXT_MAJOR,REQUIRED_BASEEXT_MINOR,REQUIRED_BASEEXT_PATCH);
const std::string VMExtension::NAME("jb2vm");

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        return new VMExtension(PASSLOC, compiler);
    }
}

VMExtension::VMExtension(LOCATION, Compiler *compiler, bool extended, std::string extensionName)
    : Extension(compiler, (extended ? extensionName : NAME)) {

    _baseExt = compiler->loadExtension<Base::BaseExtension>(PASSLOC, &requiredBaseVersion);
}

VMExtension::~VMExtension() {
}

void
VMExtension::Goto(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->Goto(PASSLOC, b, target);
}

void
VMExtension::IfCmpEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpEqualZero(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *condition) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpEqualZero(PASSLOC, b, target, condition);
}

void
VMExtension::IfCmpLessOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpLessOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpLessThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpLessThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpGreaterOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpGreaterOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpGreaterThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpGreaterThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpNotEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpNotEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpNotEqualZero(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *condition) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpNotEqualZero(PASSLOC, b, target, condition);
}

void
VMExtension::IfCmpUnsignedLessOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpUnsignedLessOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedLessThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpUnsignedLessThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedGreaterOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpUnsignedGreaterOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedGreaterThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    baseExt()->IfCmpUnsignedGreaterThan(PASSLOC, b, target, left, right);
}

BytecodeBuilder *
VMExtension::OrphanBytecodeBuilder(Base::FunctionCompilation *comp, int32_t bcIndex, int32_t bcLength, std::string name, Context *context) {
    return new BytecodeBuilder(comp, this, bcIndex, bcLength, name, context);
}

} // namespace VM
} // namespace JitBuilder
} // namespace OMR
