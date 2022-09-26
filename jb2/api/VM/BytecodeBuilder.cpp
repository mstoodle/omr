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

#include "Base/BaseExtension.hpp"
#include "Base/FunctionCompilation.hpp"
#include "BytecodeBuilder.hpp"
#include "JB1MethodBuilder.hpp"
#include "TextWriter.hpp"
#include "TypeDictionary.hpp"
#include "VirtualMachineState.hpp"
#include "VMExtension.hpp"

namespace OMR {
namespace JitBuilder {
namespace VM {

BytecodeBuilder::BytecodeBuilder(Base::FunctionCompilation *comp,
                                 VMExtension *vme,
                                 int32_t bcIndex,
                                 int32_t bcLength,
                                 std::string name,
                                 Context *context)
    : Builder(comp, context, name)
    , _vme(vme)
    , _bcIndex(bcIndex)
    , _bcLength(bcLength)
    , _initialVMState(0)
    , _vmState(0)
    , _fallThroughBuilder(0) {

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
        // there is already an established vm state at the target builder
        // so we need to synchronize this builder's vm state with the target builder's vm state
        // for example, the local variables holding the elements on the operand stack may not match
        // create an intermediate builder object to do that work
        // TODO: Compilation needs "Kind"s too
        BytecodeBuilder *intermediateBuilder = _vme->OrphanBytecodeBuilder(static_cast<Base::FunctionCompilation *>(_comp), b->bcIndex(), b->bcLength(), b->name(), b->context());

        _vmState->MergeInto(PASSLOC, b->initialVMState(), intermediateBuilder);

        // direct control to b from intermediateBuilder, but we have already propagated vm state : use BaseExtension::Goto
        _vme->baseExt()->Goto(PASSLOC, intermediateBuilder, b);
        intermediateBuilder->_successorBuilders.push_back(b);
        return intermediateBuilder; // branches should direct towards intermediateBuilder not original *b
    } else {
        b->propagateVMState(PASSLOC, _vmState);
    }
    return b;
}

void
BytecodeBuilder::writeProperties(TextWriter & w) const {
    this->Builder::writeProperties(w);

    w.indent() << "[ bcIndex " << bcIndex() << " ]" << w.endl();
    w.indent() << "[ bcLength " << bcLength() << " ]" << w.endl();

    if (_fallThroughBuilder)
        w.indent() << "[ fallThroughBuilder " << _fallThroughBuilder << " ]" << w.endl();
    else
        w.indent() << "[ fallThroughBuilder NULL ]" << w.endl();

    for (auto it=_successorBuilders.begin(); it != _successorBuilders.end(); it++) {
        BytecodeBuilder *succ = *it;
        w.indent() << "[ successorBuilder " << succ << " ]" << w.endl();
    }
}

void
BytecodeBuilder::jbgen(JB1MethodBuilder *j1mb) const {
    j1mb->createBytecodeBuilder(this, bcIndex(), name());
}

void
BytecodeBuilder::jbgenSuccessors(JB1MethodBuilder *j1mb) const {
    if (_controlReachesEnd && _fallThroughBuilder)
        j1mb->addFallThroughBuilder(this, _fallThroughBuilder);

    for (auto it = _successorBuilders.begin(); it != _successorBuilders.end(); it++) {
        BytecodeBuilder *succ = *it;
        j1mb->addSuccessorBuilder(this, succ);
    }
}

} // namespace VM
} // namespace JitBuilder
} // namespace OMR
