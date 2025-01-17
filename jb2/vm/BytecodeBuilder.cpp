/*******************************************************************************
 * Copyright (c) 2017, 2022 IBM Corp. and others
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
#include "Func/Func.hpp"
#include "Base/Base.hpp"
#include "vm/BytecodeBuilder.hpp"
#include "vm/VirtualMachineState.hpp"
#include "vm/VMExtension.hpp"
#include "vm/VMIRClonerAddon.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_REUSECAT(BytecodeBuilder, Builder)
SUBCLASS_KINDSERVICE_IMPL(BytecodeBuilder,"BytecodeBuilder",Builder,Extensible);

BytecodeBuilder::BytecodeBuilder(Allocator *a,
                                 VMExtension *vmx,
                                 IR *ir,
                                 Scope *scope,
                                 int32_t bcIndex,
                                 int32_t bcLength,
                                 String name)
    : Builder(a, vmx, KIND(Extensible), ir, scope, name)
    , _bcIndex(bcIndex)
    , _bcLength(bcLength)
    , _initialVMState(0)
    , _vmState(0)
    , _fallThroughBuilder(0)
    , _successorBuilders(NULL, ir->mem()) {

}

BytecodeBuilder::BytecodeBuilder(Allocator *a, const BytecodeBuilder *source, IRCloner *cloner)
    : Builder(a, source, cloner)
    , _bcIndex(source->_bcIndex)
    , _bcLength(source->_bcLength)
    , _initialVMState(cloner->addon<VMIRClonerAddon>()->clonedState(source->_initialVMState))
    , _vmState(cloner->addon<VMIRClonerAddon>()->clonedState(source->_vmState))
    , _fallThroughBuilder(cloner->clonedBuilder(source->_fallThroughBuilder)->refine<BytecodeBuilder>())
    , _successorBuilders(NULL, a) {

    for (auto it=source->_successorBuilders.iterator();it.hasItem(); it++) {
        BytecodeBuilder *b = it.item();
        _successorBuilders.push_back(cloner->clonedBuilder(b)->refine<BytecodeBuilder>());
    }
}

Builder *
BytecodeBuilder::clone(Allocator *mem, IRCloner *cloner) const {
    return new (mem) BytecodeBuilder(mem, this, cloner);
}

BytecodeBuilder::~BytecodeBuilder() {
    if (_vmState)
        delete _vmState;
    if (_initialVMState && _initialVMState != _vmState)
        delete _initialVMState;
}

VMExtension *
BytecodeBuilder::vmx() const {
    return ext()->refine<VMExtension>();
}

BytecodeBuilder *
BytecodeBuilder::AddFallThroughBuilder(LOCATION, BytecodeBuilder *ftb) {
    assert(_fallThroughBuilder == 0);

    BytecodeBuilder *b = ftb;
    b = transferVMState(PASSLOC, b);    // may change what b points at if transition coce is needed

    //if (b != ftb)
    //    TraceIL("IlBuilder[ %p ]:: fallThrough successor changed to [ %p ]\n", this, b);

    _fallThroughBuilder = b;
    return b;
}

BytecodeBuilder *
BytecodeBuilder::AddSuccessorBuilder(LOCATION, BytecodeBuilder *builder) {
    builder = transferVMState(PASSLOC, builder);
    // if the code below changes, make sure to check transferVMState() as it also appears in that function
    _successorBuilders.push_back(builder);   // must be the bytecode builder that comes back from transferVMState()
    return builder;
}

// AddSuccessorBuilders() should be called with a list of pointer to BytecodeBuilder *.
// Each one of these pointers could be changed by AddSuccessorBuilders() in the case where
// some operations need to be inserted along the control flow edges to synchronize the
// vm state from "this" builder to the target BytecodeBuilder. For this reason, the actual
// control flow edges should be created (i.e. with Goto, IfCmp*, etc.) *after* calling
// AddSuccessorBuilders, and the target used when creating those flow edges should take
// into account that AddSuccessorBuilders may change the builder object provided.
void
BytecodeBuilder::AddSuccessorBuilders(LOCATION, uint32_t numExits, ...) {
    va_list exits;
    va_start(exits, numExits);
    for (auto e = 0; e < numExits; e++) {
        BytecodeBuilder **pBuilder = (BytecodeBuilder **) va_arg(exits, BytecodeBuilder **);
        BytecodeBuilder *builder = *pBuilder;
        builder = AddSuccessorBuilder(PASSLOC, builder);
        if (builder != *pBuilder)
            *pBuilder = builder;
    }
    va_end(exits);
}

void
BytecodeBuilder::propagateVMState(LOCATION, VirtualMachineState *fromVMState) {
    _initialVMState = fromVMState->MakeCopy(PASSLOC, this);
    _vmState = fromVMState->MakeCopy(PASSLOC, this);
}

// transferVMState needs to be called before the actual transfer operation (Goto, IfCmp,
// etc.) is created because we may need to insert a builder object along that control
// flow edge to synchronize the vm state at the target (in the case of a merge point).
// On return, the object pointed at by the "b" parameter may have changed. The caller
// should direct control for this edge to whatever the parameter passed to "b" is
// pointing at on return
BytecodeBuilder *
BytecodeBuilder::transferVMState(LOCATION, BytecodeBuilder *b) {
    assert(_vmState != NULL);
    if (b->initialVMState()) {
        VMExtension *vx = vmx();

        // there is already an established vm state at the target builder
        // so we need to synchronize this builder's vm state with the target builder's vm state
        // for example, the local variables holding the elements on the operand stack may not match
        // create an intermediate builder object to do that work
        // TODO: Compilation needs "Kind"s too
        BytecodeBuilder *intermediateBuilder = vx->OrphanBytecodeBuilder(_ir, b->bcIndex(), b->bcLength(), b->scope(), b->name());

        _vmState->MergeInto(PASSLOC, b->initialVMState(), intermediateBuilder);

        // direct control to b from intermediateBuilder, but we have already propagated vm state : use BaseExtension::Goto
        vx->bx()->Goto(PASSLOC, intermediateBuilder, b);
        intermediateBuilder->_successorBuilders.push_back(b);
        return intermediateBuilder; // branches should direct towards intermediateBuilder not original *b
    } else {
        b->propagateVMState(PASSLOC, _vmState);
    }
    return b;
}

void
BytecodeBuilder::logProperties(TextLogger & lgr) {
    this->Builder::logProperties(lgr);

    lgr.indent() << "[ bcIndex " << bcIndex() << " ]" << lgr.endl();
    lgr.indent() << "[ bcLength " << bcLength() << " ]" << lgr.endl();

    if (_fallThroughBuilder)
        lgr.indent() << "[ fallThroughBuilder " << _fallThroughBuilder << " ]" << lgr.endl();
    else
        lgr.indent() << "[ fallThroughBuilder NULL ]" << lgr.endl();

    for (auto it=_successorBuilders.iterator(); it.hasItem(); it++) {
        BytecodeBuilder *succ = it.item();
        lgr.indent() << "[ successorBuilder " << succ << " ]" << lgr.endl();
    }
}


} // namespace VM
} // namespace JB2
} // namespace OMR
