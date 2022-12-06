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

#include "BytecodeBuilder.hpp"
#include "Compiler.hpp"
#include "VMExtension.hpp"

namespace OMR {
namespace JitBuilder {
namespace VM {

const SemanticVersion VMExtension::version(VMEXT_MAJOR,VMEXT_MINOR,VMEXT_PATCH);
const String VMExtension::NAME("jb2vm");

static const MajorID REQUIRED_BASEEXT_MAJOR=0;
static const MinorID REQUIRED_BASEEXT_MINOR=1;
static const PatchID REQUIRED_BASEEXT_PATCH=0;
static const SemanticVersion requiredBaseVersion(REQUIRED_BASEEXT_MAJOR,REQUIRED_BASEEXT_MINOR,REQUIRED_BASEEXT_PATCH);
extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Base::BaseExtension *bx = compiler->loadExtension<Base::BaseExtension>(PASSLOC, &requiredBaseVersion);
        if (bx == NULL)
            return NULL;
        return new VMExtension(PASSLOC, compiler);
    }
}

VMExtension::VMExtension(LOCATION, Compiler *compiler, bool extended, String extensionName)
    : Extension(PASSLOC, compiler, (extended ? extensionName : NAME)) {

    _bx = compiler->lookupExtension<Base::BaseExtension>();
    _fx = compiler->lookupExtension<Func::FunctionExtension>();
}

VMExtension::~VMExtension() {
}

void
VMExtension::Goto(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->Goto(PASSLOC, b, target);
}

void
VMExtension::IfCmpEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpEqualZero(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *condition) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpEqualZero(PASSLOC, b, target, condition);
}

void
VMExtension::IfCmpLessOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpLessOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpLessThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpLessThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpGreaterOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpGreaterOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpGreaterThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpGreaterThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpNotEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpNotEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpNotEqualZero(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *condition) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpNotEqualZero(PASSLOC, b, target, condition);
}

void
VMExtension::IfCmpUnsignedLessOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedLessOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedLessThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedLessThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedGreaterOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedGreaterOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedGreaterThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right) {
    target = b->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedGreaterThan(PASSLOC, b, target, left, right);
}

BytecodeBuilder *
VMExtension::OrphanBytecodeBuilder(Base::BaseCompilation *comp, int32_t bcIndex, int32_t bcLength, Context *context, String name) {
    return new BytecodeBuilder(comp, this, bcIndex, bcLength, context, name);
}

} // namespace VM
} // namespace JitBuilder
} // namespace OMR
