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
        
#ifndef VIRTUALMACHINEOPERANDSTACK_INCL
#define VIRTUALMACHINEOPERANDSTACK_INCL

#include <stdint.h>
#include <stddef.h>
#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"
#include "VirtualMachineState.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

class VirtualMachineRegister;

class VirtualMachineOperandStack: public VirtualMachineState {
    JBALLOC_(VirtualMachineOperandStack)

public:
    VirtualMachineOperandStack(MEM_LOCATION(a), VMExtension *vmx, Compilation * comp, int32_t sizeHint, VirtualMachineRegister * stackTopRegister, const Type *elementType, bool growsUp=true, int32_t stackInitialOffset=-1);
    VirtualMachineOperandStack(MEM_LOCATION(a), VirtualMachineOperandStack * other);

    virtual void Commit(LOCATION, Builder * b);
    virtual void Reload(LOCATION, Builder * b);
    virtual VirtualMachineState * MakeCopy(LOCATION, Builder *b);
    virtual void MergeInto(LOCATION, VirtualMachineState * vmState, Builder * b);

    void Drop(int32_t depth);
    void Dup();
    Value * Pick(int32_t depth);
    Value * Pop();
    void Push(Value * value);
    Value * Top();
    void UpdateStack(LOCATION, Builder * b, Value * array);

protected:
    VirtualMachineOperandStack(Allocator *a, const VirtualMachineOperandStack *source, IRCloner *cloner);
    virtual VirtualMachineState *clone(Allocator *mem, IRCloner *cloner) const;

    void init(LOCATION);
    void checkSizeAndGrowIfNeeded();
    void grow(int32_t growAmount=0);

    Compilation * _comp;
    VirtualMachineRegister * _stackTopRegister;
    const Type * _elementType;
    bool _growsUp;

    int32_t _stackOffset;
    int32_t _stackMax;
    int32_t _stackTop;
    Func::LocalSymbol *_stackBaseLocal;
    int32_t _pushAmount;
    Value ** _stack;

    SUBCLASS_KINDSERVICE_DECL(Extensible, VirtualMachineOperandStack);
};

} // namespace VM
} // namespace JB2
} // namespace OMR

#endif // VIRTUALMACHINEOPERANDSTACK_INCL
