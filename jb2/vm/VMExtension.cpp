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

#include "JBCore.hpp"
#include "vm/VMBuilderAddon.hpp"
#include "vm/VMExtension.hpp"
#include "vm/VMIRClonerAddon.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_REUSECAT(VMExtension, Extension);
SUBCLASS_KINDSERVICE_IMPL(VMExtension,"VMExtension",Extension,Extensible);

const SemanticVersion VMExtension::version(VMEXT_MAJOR,VMEXT_MINOR,VMEXT_PATCH);
const String VMExtension::NAME("jb2vm");

static const MajorID REQUIRED_BASEEXT_MAJOR=0;
static const MinorID REQUIRED_BASEEXT_MINOR=1;
static const PatchID REQUIRED_BASEEXT_PATCH=0;
static const SemanticVersion requiredBaseVersion(REQUIRED_BASEEXT_MAJOR,REQUIRED_BASEEXT_MINOR,REQUIRED_BASEEXT_PATCH);

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Base::BaseExtension *bx = compiler->loadExtension<Base::BaseExtension>(PASSLOC, Base::BaseExtension::NAME, &requiredBaseVersion);
        if (bx == NULL)
            return NULL;
        Allocator *mem = compiler->mem();
        return new (mem) VMExtension(MEM_PASSLOC(mem), compiler);
    }
}

VMExtension::VMExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(MEM_PASSLOC(a), CLASSKIND(VMExtension,Extensible), compiler, (extended ? extensionName : NAME))
    , _bx(compiler->lookupExtension<Base::BaseExtension>())
    , _fx(compiler->lookupExtension<Func::FunctionExtension>())
    , aGoto(_bx->aGoto)
    , aIfCmpEqual(_bx->aIfCmpEqual)
    , aIfCmpEqualZero(_bx->aIfCmpEqualZero)
    , aIfCmpLessOrEqual(_bx->aIfCmpLessOrEqual)
    , aIfCmpLessThan(_bx->aIfCmpLessThan)
    , aIfCmpGreaterOrEqual(_bx->aIfCmpGreaterOrEqual)
    , aIfCmpGreaterThan(_bx->aIfCmpGreaterThan)
    , aIfCmpNotEqual(_bx->aIfCmpNotEqual)
    , aIfCmpNotEqualZero(_bx->aIfCmpNotEqualZero)
    , aIfCmpUnsignedLessOrEqual(_bx->aIfCmpUnsignedLessOrEqual)
    , aIfCmpUnsignedLessThan(_bx->aIfCmpUnsignedLessThan)
    , aIfCmpUnsignedGreaterOrEqual(_bx->aIfCmpUnsignedGreaterOrEqual)
    , aIfCmpUnsignedGreaterThan(_bx->aIfCmpUnsignedGreaterThan) {

    registerForExtensible(CLASSKIND(IRCloner,Extensible), this);
    registerForExtensible(CLASSKIND(Builder,Extensible), this);
}

VMExtension::~VMExtension() {
}

void
VMExtension::createAddon(Extensible *e) {
    Allocator *mem = e->allocator();
    if (e->isKind<Builder>()) {
        VMBuilderAddon *vmba = new (mem) VMBuilderAddon(mem, this, e->refine<Builder>());
        e->attach(vmba);
    } else {
        assert(e->isKind<IRCloner>());
        VMIRClonerAddon *vc = new (mem) VMIRClonerAddon(mem, this, e->refine<IRCloner>());
        e->attach(vc);
    }
}

void
VMExtension::Goto(LOCATION, Builder *b, Builder *target) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->Goto(PASSLOC, b, target);
}

void
VMExtension::IfCmpEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpEqualZero(LOCATION, Builder *b, Builder *target, Value *condition) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpEqualZero(PASSLOC, b, target, condition);
}

void
VMExtension::IfCmpLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpLessOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpLessThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpGreaterOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpGreaterThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpNotEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpNotEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpNotEqualZero(LOCATION, Builder *b, Builder *target, Value *condition) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpNotEqualZero(PASSLOC, b, target, condition);
}

void
VMExtension::IfCmpUnsignedLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedLessOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedLessThan(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedGreaterOrEqual(PASSLOC, b, target, left, right);
}

void
VMExtension::IfCmpUnsignedGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    target = b->addon<VMBuilderAddon>()->AddSuccessorBuilder(PASSLOC, target);
    bx()->IfCmpUnsignedGreaterThan(PASSLOC, b, target, left, right);
}

Builder *
VMExtension::EntryBuilder(LOCATION, IR *ir, Scope *scope, String name) {
    Builder *b = this->Extension::EntryBuilder(PASSLOC, ir, scope, name);
    VMBuilderAddon *vmba = b->addon<VMBuilderAddon>();
    vmba->setBCIndex(-1);
    vmba->setBCLength(-1);
    vmba->setVMState(NULL); // expected to be either unused or updated later by caller
    return b;
}

Builder *
VMExtension::OrphanBuilder(LOCATION, Builder *parent, int32_t bcIndex, int32_t bcLength, Scope *scope, String name) {
    Builder *b = this->Extension::OrphanBuilder(PASSLOC, parent, scope, name);
    VMBuilderAddon *vmba = b->addon<VMBuilderAddon>();
    vmba->setBCIndex(bcIndex);
    vmba->setBCLength(bcLength);
    return b;
}

CompiledBody *
VMExtension::compile(LOCATION, Func::Function *func, StrategyID strategy, TextLogger *lgr) {

    if (strategy == NoStrategy)
        strategy = compiler()->coreExt()->strategyCodegen;

    Allocator *mem = compiler()->mem();
    Func::FunctionCompilation *comp = new (mem) Func::FunctionCompilation(mem, this, func, strategy);

    setLogger(comp, lgr);

    CompiledBody *body = _compiler->compile(PASSLOC, comp, strategy);

    delete comp;
    return body;
}

} // namespace VM
} // namespace JB2
} // namespace OMR
