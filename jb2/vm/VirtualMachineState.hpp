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
        
#ifndef VIRTUALMACHINESTATE_INCL
#define VIRTUALMACHINESTATE_INCL

#include "stdint.h"
#include "stddef.h"
#include <map>
#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Func/Func.hpp"
#include "VMExtension.hpp"

namespace OMR {
namespace JB2 {

class Builder;

namespace VM {

class BytecodeBuilder;
class VMIRClonerAddon;

typedef uint64_t VirtualMachineStateID;
const VirtualMachineStateID NoVirtualMachineStateID=0;

class VirtualMachineState : public Extensible {
    JBALLOC_(VirtualMachineState)

    friend class VMIRClonerAddon;

public:
    DYNAMIC_ALLOC_ONLY(VirtualMachineState, LOCATION, VMExtension *vmx, KINDTYPE(Extensible) kind)
        : Extensible(a, vmx, kind)
        , _id(nextVirtualMachineStateID++)
        , _createLocation(PASSLOC) {

    }

    VirtualMachineStateID id() const { return _id; }
    CreateLocation createLocation() const { return _createLocation; }
    Base::BaseExtension *bx() const { return vmx()->bx(); }
    Func::FunctionExtension *fx() const { return vmx()->fx(); }
    VMExtension *vmx() const { return ext()->refine<VMExtension>(); }

    virtual void Commit(LOCATION, Builder * b) { }
    virtual VirtualMachineState * MakeCopy(LOCATION, Builder *b);
    virtual void MergeInto(LOCATION, VirtualMachineState * vmState, Builder * b) { }
    virtual void Reload(LOCATION, Builder * b) { }

protected:
    VirtualMachineState(Allocator *a, const VirtualMachineState *source, IRCloner *cloner);
    virtual VirtualMachineState *clone(Allocator *mem, IRCloner *cloner) const;

    VirtualMachineStateID _id;
    CreateLocation _createLocation;

    static VirtualMachineStateID nextVirtualMachineStateID;

    SUBCLASS_KINDSERVICE_DECL(Extensible, VirtualMachineState);
};

} // namespace VM
} // namespace JB2
} // namespace OMR

#endif // !defined(VIRTUALMACHINESTATE_INCL)
