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
        
#ifndef BYTECODEBUILDER_INCL
#define BYTECODEBUILDER_INCL

#include <stdint.h>
#include <stddef.h>
#include "JBCore.hpp"
#include "Base/Base.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

class VirtualMachineState;
class VMExtension;

class BytecodeBuilder: public Builder {
    JBALLOC_(BytecodeBuilder)

    friend VMExtension;

public:
    DYNAMIC_ALLOC_ONLY(BytecodeBuilder, VMExtension *vmx, IR *ir, Scope *scope, int32_t bcIndex, int32_t bcLength=1, String name="");
    int32_t bcIndex() const { return _bcIndex; }
    int32_t bcLength() const { return _bcLength; }

    void setVMState(VirtualMachineState *state) { _vmState = state; }
    VirtualMachineState * initialVMState() const { return _initialVMState; }
    VirtualMachineState * vmState() const { return _vmState; }

    void propagateVMState(LOCATION, VirtualMachineState *fromState);

    virtual String logName() const { return String(allocator(), "BytecodeBuilder"); }
    virtual void logProperties(TextLogger & lgr);

protected:
    BytecodeBuilder(Allocator *a, const BytecodeBuilder *source, IRCloner *cloner);

    virtual Builder *clone(Allocator *mem, IRCloner *cloner) const;

    // no longer need clients to call these; they are called directly by control flow operations created by VMExtension
    VMExtension *vmx() const;

    BytecodeBuilder * AddFallThroughBuilder(LOCATION, BytecodeBuilder *ftb);
    BytecodeBuilder * AddSuccessorBuilder(LOCATION, BytecodeBuilder *b);
    void AddSuccessorBuilders(LOCATION, uint32_t numBuilders, ...);

    BytecodeBuilder *transferVMState(LOCATION, BytecodeBuilder *b);

    int32_t _bcIndex;
    int32_t _bcLength;
    VirtualMachineState * _initialVMState;
    VirtualMachineState * _vmState;

    BytecodeBuilder * _fallThroughBuilder;
    List<BytecodeBuilder *> _successorBuilders;

    SUBCLASS_KINDSERVICE_DECL(Extensible, BytecodeBuilder);
};

} // VM
} // JB2
} // OMR

#endif // BYTECODEBUILDER_INCL
