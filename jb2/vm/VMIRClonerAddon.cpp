/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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

#include "vm/VirtualMachineState.hpp"
#include "vm/VMIRClonerAddon.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_REUSECAT(VMIRClonerAddon, IRCloner)
SUBCLASS_KINDSERVICE_IMPL(VMIRClonerAddon,"VMIRClonerAddon",Addon,Extensible)

VMIRClonerAddon::VMIRClonerAddon(Allocator *a, VMExtension *vmx, IRCloner *root)
    : Addon(a, vmx, root, KIND(Extensible))
    , _clonedStates(NULL, a) {

}

VMIRClonerAddon::~VMIRClonerAddon() {
    for (int i=0;i < _clonedStates.length(); i++) {
        VirtualMachineState *s = _clonedStates[i];
        delete s;
    }
}

VMExtension *
VMIRClonerAddon::vmx() const {
    return ext()->refine<VMExtension>();
}

VirtualMachineState *
VMIRClonerAddon::clonedState(VirtualMachineState *s) {
    VirtualMachineStateID id=s->id();
    VirtualMachineState *clonedState = NULL;
    if (id < _clonedStates.length())
        clonedState = _clonedStates[id];

    if (clonedState == NULL) {
        IRCloner *cloner = root()->refine<IRCloner>();
        clonedState = s->clone(cloner->allocator(), cloner);
        _clonedStates.assign(id, clonedState);
    }

    return clonedState;
}

} // namespace VM
} // namespace JB2
} // namespace OMR
