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

#include "stdint.h"
#include "stddef.h"
#include "VirtualMachineState.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Type;
class Value;

namespace Base { class BytecodeBuilder; }
namespace Base { class Function; }
namespace Base { class LocalSymbol; }

namespace VM {

class VirtualMachineRegister;

class VirtualMachineOperandStack: public VirtualMachineState {
public:
    VirtualMachineOperandStack(LOCATION, VMExtension *vme, Base::Function * func, int32_t sizeHint, VirtualMachineRegister * stackTopRegister, const Type *elementType, bool growsUp=true, int32_t stackInitialOffset=-1);
    VirtualMachineOperandStack(LOCATION, VirtualMachineOperandStack * other);

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

    static const StateKind getStateClassKind();

protected:
    void init(LOCATION);
    void checkSizeAndGrowIfNeeded();
    void grow(int32_t growAmount=0);

    Base::Function * _func;
    VirtualMachineRegister * _stackTopRegister;
    const Type * _elementType;
    bool _growsUp;

    int32_t _stackOffset;
    int32_t _stackMax;
    int32_t _stackTop;
    Base::LocalSymbol *_stackBaseLocal;
    int32_t _pushAmount;
    Value ** _stack;

    static StateKind STATEKIND;
    static bool kindRegistered;
};

} // namespace VM
} // namespace JitBuilder
} // namespace OMR

#endif // VIRTUALMACHINEOPERANDSTACK_INCL
