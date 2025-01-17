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
        
#ifndef VIRTUALMACHINEREGISTER_INCL
#define VIRTUALMACHINEREGISTER_INCL

#include <stddef.h>
#include <stdint.h>
#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Func/Func.hpp"
#include "VirtualMachineState.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

// forward declarations for all API classes
class BytecodeBuilder;

/**
 * @brief simulate virtual machine state variable via an address
 *
 * VirtualMachineRegister can be used to represent values in the virtual machine
 * at any address. The value does not need to be a virtual machine register, but
 * often it is the registers of the virtual machine that are candidates for
 * VirtualMachineRegister. An alternative is VirtualMachineRegisterInStruct,
 * which may be more convenient if the virtual machine value is stored in a
 * structure that the compiled method has easy access to (for example, if the
 * base address of the struct is a parameter of every compiled method such as
 * a "thread" structure or a "frame" structure).
 *
 * The simulated register value is simply stored in a single local variable in the
 * native stack frame, which gives the compiler visibility to all changes to the
 * register (and enables dataflow optimization / simplification). Because there is
 * just a single local variable, the CopyState() and MergeInto() functions do not
 * need to do anything (the value can accessible from that variable at all program
 * locations). The Commit() and Reload() functions simply move the value back and
 * forth between the local variable and the address of the actual virtual machine
 * state variable.
 *
 * VirtualMachineRegister provides four additional functions:
 * Adjust() adds the given Value to the "simualted" value of the register (two
 * versions: one adjusts by a Value and the other by a constant amount)
 * Load() will load the *simulated* value of the register for use in the builder "b"
 * Store() will store the provided "value" into the *simulated* register by
 * appending to the builder "b"
 */

class VirtualMachineRegister : public VirtualMachineState {
    JBALLOC_NO_DESTRUCTOR_(VirtualMachineRegister)

public:
   /**
    * @brief public constructor used to create a virtual machine state variable
    * @param vme a pointer to the active VM extension
    * @param name a string containing the register's name
    * @param func the function being compiled
    * @param addressOfRegister is the address of the actual register
    * @param doReload whether local register should be Reloaded (default true, MakeCopy will pass false)
    */
    VirtualMachineRegister(MEM_LOCATION(a), VMExtension *vmx, String name, Compilation *comp, Value * addressOfRegister, bool doReload=true);
    virtual ~VirtualMachineRegister() { }

    // virtualmachinestate api

    /**
     * @brief write the simulated register value to the virtual machine
     * @param b the builder where the operation will be placed to store the virtual machine value
     */
    virtual void Commit(LOCATION, Builder * b);

    virtual VirtualMachineState * MakeCopy(LOCATION, Builder *b);

    /**
     * @brief since the same local variable is used in all states, nothing is needed to merge one state into another
     * @param vmState 
     * @param b 
     */
    virtual void MergeInto(LOCATION, VirtualMachineState * vmState, Builder * b) { }

    /**
     * @brief transfer the current virtual machine register value to the simulated local variable
     * @param b the builder where the operation will be placed to load the virtual machine value
     */
    virtual void Reload(LOCATION, Builder * b);

    // virtualmachineregister api
    void Adjust(LOCATION, Builder * b, size_t amount);
    void Adjust(LOCATION, Builder * b, Value * amount);
    Value * Load(LOCATION, Builder * b);
    void Store(LOCATION, Builder * b, Value * value);

protected:
    /**
     * @brief constructor can be used by subclasses to initialize just the LOCATION, local, and override kind
     * @param vmx the VM extension to use
     * @param func the function being compiled
     */
    VirtualMachineRegister(MEM_LOCATION(a), VMExtension *vmx, String name, Compilation * comp, KINDTYPE(Extensible) kind);

    VirtualMachineRegister(Allocator *a, const VirtualMachineRegister *source, IRCloner *cloner);
    virtual VirtualMachineState *clone(Allocator *mem, IRCloner *cloner) const;

    String _name;
    Compilation *_comp;
    Func::LocalSymbol * _local;
    uint32_t _adjustByStep;
    Value * _addressOfRegister;
    const Base::PointerType *_pRegisterType;
    const Type *_integerTypeForAdjustments;
    bool _isAdjustable;

    SUBCLASS_KINDSERVICE_DECL(Extensible, VirtualMachineRegister);
};

} // VM
} // JB2
} // OMR

#endif // !defined(VIRTUALMACHINEREGISTER_INCL)
