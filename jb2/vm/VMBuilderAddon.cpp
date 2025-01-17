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
#include "vm/VirtualMachineState.hpp"
#include "vm/VMAddon.hpp"
#include "vm/VMBuilderAddon.hpp"
#include "vm/VMExtension.hpp"
#include "vm/VMIRClonerAddon.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_REUSECAT(VMBuilderAddon, VMAddonIR)
SUBCLASS_KINDSERVICE_IMPL(VMBuilderAddon,"VMBuilderAddon",VMAddonIR,Extensible);

VMBuilderAddon::VMBuilderAddon(Allocator *a,
                               VMExtension *vmx,
                               Builder *root)
    : VMAddonIR(a, vmx, root, KIND(Extensible))
    , _bcIndex(-1)
    , _bcLength(-1)
    , _initialVMState(0)
    , _vmState(0)
    , _fallThroughBuilder(0)
    , _successorBuilders(NULL, root->ir()->mem()) {

}

VMBuilderAddon::VMBuilderAddon(Allocator *a, const VMBuilderAddon *source, IRCloner *cloner)
    : VMAddonIR(a, source, cloner)
    , _bcIndex(source->_bcIndex)
    , _bcLength(source->_bcLength)
    , _initialVMState(cloner->addon<VMIRClonerAddon>()->clonedState(source->_initialVMState))
    , _vmState(cloner->addon<VMIRClonerAddon>()->clonedState(source->_vmState))
    , _fallThroughBuilder(cloner->clonedBuilder(source->_fallThroughBuilder))
    , _successorBuilders(NULL, a) {

    for (auto it=source->_successorBuilders.iterator();it.hasItem(); it++) {
        Builder *b = it.item();
        _successorBuilders.push_back(cloner->clonedBuilder(b));
    }
}

AddonIR *
VMBuilderAddon::clone(Allocator *mem, IRCloner *cloner) const {
    return new (mem) VMBuilderAddon(mem, this, cloner);
}

VMBuilderAddon::~VMBuilderAddon() {
    if (_vmState)
        delete _vmState;
    if (_initialVMState && _initialVMState != _vmState)
        delete _initialVMState;
}

Builder *
VMBuilderAddon::AddFallThroughBuilder(LOCATION, Builder *ftb) {
    assert(_fallThroughBuilder == 0);

    Builder *b = ftb;
    b = transferVMState(PASSLOC, b);    // may change what b points at if transition coce is needed

    //if (b != ftb)
    //    TraceIL("IlBuilder[ %p ]:: fallThrough successor changed to [ %p ]\n", this, b);

    _fallThroughBuilder = b;
    return b;
}

Builder *
VMBuilderAddon::AddSuccessorBuilder(LOCATION, Builder *builder) {
    builder = transferVMState(PASSLOC, builder);
    // if the code below changes, make sure to check transferVMState() as it also appears in that function
    _successorBuilders.push_back(builder);   // must be the bytecode builder that comes back from transferVMState()
    return builder;
}

// AddSuccessorBuilders() should be called with a list of pointer to Builder *.
// Each one of these pointers could be changed by AddSuccessorBuilders() in the case where
// some operations need to be inserted along the control flow edges to synchronize the
// vm state from "this" builder to the target Builder. For this reason, the actual
// control flow edges should be created (i.e. with Goto, IfCmp*, etc.) *after* calling
// AddSuccessorBuilders, and the target used when creating those flow edges should take
// into account that AddSuccessorBuilders may change the builder object provided.
void
VMBuilderAddon::AddSuccessorBuilders(LOCATION, uint32_t numExits, ...) {
    va_list exits;
    va_start(exits, numExits);
    for (auto e = 0; e < numExits; e++) {
        Builder **pBuilder = (Builder **) va_arg(exits, Builder **);
        Builder *builder = *pBuilder;
        builder = AddSuccessorBuilder(PASSLOC, builder);
        if (builder != *pBuilder)
            *pBuilder = builder;
    }
    va_end(exits);
}

void
VMBuilderAddon::propagateVMState(LOCATION, VirtualMachineState *fromVMState) {
    Builder *b = root()->refine<Builder>();
    _initialVMState = fromVMState->MakeCopy(PASSLOC, b);
    _vmState = fromVMState->MakeCopy(PASSLOC, b);
}

// transferVMState needs to be called before the actual transfer operation (Goto, IfCmp,
// etc.) is created because we may need to insert a builder object along that control
// flow edge to synchronize the vm state at the target (in the case of a merge point).
// On return, the object pointed at by the "b" parameter may have changed. The caller
// should direct control for this edge to whatever the parameter passed to "b" is
// pointing at on return
Builder *
VMBuilderAddon::transferVMState(LOCATION, Builder *btgt) {
    assert(_vmState != NULL);
    Builder *b = root()->refine<Builder>();
    VMBuilderAddon *btgt_vmba = btgt->addon<VMBuilderAddon>();
    assert(btgt_vmba);
    if (btgt_vmba->initialVMState()) {
        VMExtension *vx = vmx();

        // there is already an established vm state at btgt
        // so we need to synchronize this builder's vm state with the target builder's vm state
        // for example, the local variables holding the elements on the operand stack may not match
        // create an intermediate builder object to do that work to synchronize the vm state
        Builder *intermediateBuilder = vx->OrphanBuilder(PASSLOC, btgt->parent(), btgt_vmba->bcIndex(), btgt_vmba->bcLength(), btgt->scope(), btgt->name());
        _vmState->MergeInto(PASSLOC, btgt_vmba->initialVMState(), intermediateBuilder);

        // direct control to btgt from intermediateBuilder, but we have already propagated vm state : use BaseExtension::Goto
        vx->bx()->Goto(PASSLOC, intermediateBuilder, btgt);
        intermediateBuilder->addon<VMBuilderAddon>()->_successorBuilders.push_back(btgt);
        btgt = intermediateBuilder; // branches should direct towards intermediateBuilder not original b
    } else {
        btgt_vmba->propagateVMState(PASSLOC, _vmState);
    }
    return btgt;
}

void
VMBuilderAddon::logProperties(TextLogger & lgr) {
    // how does this work now?
    //this->root()->logProperties(lgr);

    lgr.indent() << "[ bcIndex " << bcIndex() << " ]" << lgr.endl();
    lgr.indent() << "[ bcLength " << bcLength() << " ]" << lgr.endl();

    if (_fallThroughBuilder)
        lgr.indent() << "[ fallThroughBuilder " << _fallThroughBuilder << " ]" << lgr.endl();
    else
        lgr.indent() << "[ fallThroughBuilder NULL ]" << lgr.endl();

    for (auto it=_successorBuilders.iterator(); it.hasItem(); it++) {
        Builder *succ = it.item();
        lgr.indent() << "[ successorBuilder " << succ << " ]" << lgr.endl();
    }
}


} // namespace VM
} // namespace JB2
} // namespace OMR
