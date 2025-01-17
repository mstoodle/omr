/*******************************************************************************
 * Copyright (c) 2016, 2018 IBM Corp. and others
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

#include "VirtualMachineState.hpp"
#include "Builder.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC(VirtualMachineState);
SUBCLASS_KINDSERVICE_IMPL(VirtualMachineState,"VirtualMachineState",Extensible,Extensible);

VirtualMachineStateID VirtualMachineState::nextVirtualMachineStateID = NoVirtualMachineStateID+1;

VirtualMachineState::VirtualMachineState(Allocator *a, const VirtualMachineState *source, IRCloner *cloner)
    : Extensible(a, source->ext(), source->_kind)
    , _id(source->_id)
    , _createLocation(source->_createLocation) {

}

VirtualMachineState *
VirtualMachineState::clone(Allocator *mem, IRCloner *cloner) const {
     return new (mem) VirtualMachineState(mem, this, cloner);
}

VirtualMachineState::~VirtualMachineState() {

}

VirtualMachineState *
VirtualMachineState::MakeCopy(LOCATION, Builder *b) {
    Allocator *mem = allocator();
    return new (mem) VirtualMachineState(MEM_PASSLOC(mem), vmx(), CLASSKIND(VirtualMachineState,Extensible));
}

} // namespace VM
} // namespace JB2
} // namespace OMR
