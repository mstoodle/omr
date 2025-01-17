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
        
#ifndef VIRTUALMACHINEREGISTERINSTRUCT_INCL
#define VIRTUALMACHINEREGISTERINSTRUCT_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"
#include "VirtualMachineRegister.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

/**
 * @brief used to represent virtual machine variables that are maintained in a structure stored in a local variable, such as a thread or frame object passed as a parameter to the method
 *
 * The value does not need to be a virtual machine register, but often it is the registers
 * of the virtual machine that are candidates for VirtualMachineRegisterInStruct. An
 * alternative is VirtualMachineRegister, which can be more convenient if the virtual
 * machine value is stored in a more arbitrary place or in a structure that isn't readily
 * accessible inside the compiled method.
 * VirtualMachineRegisterInStruct is a subclass of VirtualMachineRegister
 *
 * The simulated register value is simply stored in a single local variable, which
 * gives the compiler visibility to all changes to the register (and enables
 * optimization / simplification). Because there is just a single local variable,
 * the Merge() function does not need to do anything (the value is accessible from
 * the same location at all locations). The Commit() and Reload() functions simply
 * move the value back and forth between the local variable and the structure that
 * holds the actual virtual machine state.
 */

class VirtualMachineRegisterInStruct : public VirtualMachineRegister {
    JBALLOC_NO_DESTRUCTOR_(VirtualMachineRegisterInStruct)

public:
    /**
     * @brief public constructor used to create a virtual machine state variable from struct
     * @param vme VMExtension object to use
     * @param name the name of the register
     * @param func the function being compiled
     * @param fieldType type of the field that holds the virtual machine state variable
     * @param localHoldingStructAddress is the local variable symbol that holds the struct base address; it must have been stored in this symbol before control will reach the builder "b"
     * @param doReload do a Reload on every entry builder for _func (defaults true, MakeCopy passes false)
     */
    VirtualMachineRegisterInStruct(MEM_LOCATION(a), VMExtension *vmx, String name, Compilation *comp, const Base::FieldType *fieldType, Func::LocalSymbol * localHoldingStructAddress, bool doReload=true);
    virtual ~VirtualMachineRegisterInStruct() { }

    virtual void Commit(LOCATION, Builder * b);
    virtual VirtualMachineState * MakeCopy(LOCATION, Builder *b);
    // reuses MergeInto() from VirtualMachineRegister
    virtual void Reload(LOCATION, Builder * b);

protected:
    VirtualMachineRegisterInStruct(Allocator *a, const VirtualMachineRegisterInStruct *source, IRCloner *cloner);
    virtual VirtualMachineState *clone(Allocator *mem, IRCloner *cloner) const;

    const Base::FieldType * _fieldType;
    Func::LocalSymbol * _localHoldingStructAddress;

    SUBCLASS_KINDSERVICE_DECL(Extensible,VirtualMachineRegisterInStruct);
};

} // namespace VM
} // namespace JB2
} // namespace OMR

#endif // VIRTUALMACHINEREGISTERINSTRUCT_INCL
