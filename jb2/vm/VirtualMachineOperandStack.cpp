/*******************************************************************************
 * Copyright (c) 2016, 2019 IBM Corp. and others
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

#include <cstring>
#include "JBCore.hpp"
#include "VirtualMachineOperandStack.hpp"
#include "VirtualMachineRegister.hpp"
#include "VMIRClonerAddon.hpp"


namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_REUSECAT(VirtualMachineOperandStack, VirtualMachineState);
SUBCLASS_KINDSERVICE_IMPL(VirtualMachineOperandStack,"VirtualMachineOperandStack",VirtualMachineState,Extensible);


VirtualMachineOperandStack::VirtualMachineOperandStack(MEM_LOCATION(a),
                                                       VMExtension *vmx,
                                                       Compilation *comp,
                                                       int32_t sizeHint,
                                                       VirtualMachineRegister *stackTopRegister,
                                                       const Type *elementType,
                                                       bool growsUp,
                                                       int32_t stackInitialOffset)
    : VirtualMachineState(MEM_PASSLOC(a), vmx, CLASSKIND(VirtualMachineOperandStack,Extensible))
    , _comp(comp)
    , _stackTopRegister(stackTopRegister)
    , _elementType(elementType)
    , _stackOffset(stackInitialOffset)
    , _stackMax(sizeHint)
    , _stackTop(-1)
    , _stackBaseLocal(NULL)
    , _pushAmount(growsUp ? +1 : -1)
    , _stack(NULL) {

    init(PASSLOC);
}

VirtualMachineOperandStack::VirtualMachineOperandStack(MEM_LOCATION(a), VirtualMachineOperandStack *other)
    : VirtualMachineState(MEM_PASSLOC(a), other->vmx(), CLASSKIND(VirtualMachineOperandStack,Extensible))
    , _comp(other->_comp)
    , _stackTopRegister(other->_stackTopRegister)
    , _elementType(other->_elementType)
    , _stackOffset(other->_stackOffset)
    , _stackMax(other->_stackMax)
    , _stackTop(other->_stackTop)
    , _stackBaseLocal(other->_stackBaseLocal)
    , _pushAmount(other->_pushAmount)
    , _stack(NULL) {

    _stack = a->allocate<Value *>(_stackMax);
    int32_t numBytes = _stackMax * sizeof(Value *);
    memcpy(_stack, other->_stack, numBytes);
}

VirtualMachineOperandStack::VirtualMachineOperandStack(Allocator *a, const VirtualMachineOperandStack *source, IRCloner *cloner)
    : VirtualMachineState(a, source, cloner)
    , _comp(source->_comp)
    , _stackTopRegister(cloner->addon<VMIRClonerAddon>()->clonedState(source->_stackTopRegister)->refine<VirtualMachineRegister>())
    , _elementType(cloner->clonedType(source->_elementType))
    , _stackOffset(source->_stackOffset)
    , _stackMax(source->_stackMax)
    , _stackTop(source->_stackTop)
    , _stackBaseLocal(cloner->clonedSymbol(source->_stackBaseLocal)->refine<Func::LocalSymbol>())
    , _pushAmount(source->_pushAmount)
    , _stack(NULL) {

    _stack = a->allocate<Value *>(_stackMax);
    for (int i=0;i < _stackMax;i++)
        _stack[i] = cloner->clonedValue(source->_stack[i]);
}

VirtualMachineState *
VirtualMachineOperandStack::clone(Allocator *mem, IRCloner *cloner) const {
     return new (mem) VirtualMachineOperandStack(mem, this, cloner);
}

VirtualMachineOperandStack::~VirtualMachineOperandStack() {
    if (_stack) {
        allocator()->deallocate(_stack);
    }
}

// commits the simulated operand stack of values to the virtual machine state
// the given builder object is where the operations to commit the state will be inserted
// the top of the stack is assumed to be managed independently, most likely
//    as a VirtualMachineRegister or a VirtualMachineRegisterInStruct
void
VirtualMachineOperandStack::Commit(LOCATION, Builder *b) {
    Base::BaseExtension *bx = this->bx();
    Func::FunctionExtension *fx = this->fx();

    Value *stack = fx->Load(PASSLOC, b, _stackBaseLocal);

    // Adjust the vm _stackTopRegister by number of elements that have been pushed onto the stack.
    // _stackTop is -1 at 0 pushes, 0 for 1 push, so # of elements to adjust by is _stackTop+1
    _stackTopRegister->Store(PASSLOC, b, stack);
    _stackTopRegister->Adjust(PASSLOC, b, (_stackTop+1)*_pushAmount);

    for (int32_t i = _stackTop;i >= 0;i--) {
        Value *element = Pick(_stackTop - i);
        bx->StoreArray(PASSLOC, b, stack, i - _stackOffset, element);
    }
}

void
VirtualMachineOperandStack::Reload(LOCATION, Builder* b) {
    Base::BaseExtension *bx = this->bx();
    Func::FunctionExtension *fx = this->fx();
    Value *stack = fx->Load(PASSLOC, b, _stackBaseLocal);
    // reload the elements back into the simulated operand stack
    // If the # of stack element has changed, the user should adjust the # of elements
    // using Drop beforehand to add/delete stack elements.
    for (int32_t i = _stackTop; i >= 0; i--) {
        _stack[i] = bx->LoadArray(PASSLOC, b, stack, i - _stackOffset);
    }
}

// Allocate a new operand stack and copy everything in this state
// If VirtualMachineOperandStack is subclassed, this function *must* also be implemented in the subclass!
VirtualMachineState *
VirtualMachineOperandStack::MakeCopy(LOCATION, Builder *b) {
    Allocator *mem = allocator();
    return new (mem) VirtualMachineOperandStack(MEM_PASSLOC(mem), this);
}

void
VirtualMachineOperandStack::MergeInto(LOCATION, VirtualMachineState *o, Builder* b) {
    assert(o->isKind<VirtualMachineOperandStack>());
    CoreExtension *cx = compiler()->lookupExtension<CoreExtension>();
    VirtualMachineOperandStack *other = o->refine<VirtualMachineOperandStack>();
    assert(_stackTop == other->_stackTop);
    for (int32_t i=_stackTop;i >= 0;i--) {
        // only need to do something if the two entries aren't already the same
        if (other->_stack[i] != _stack[i]) {
            // what if types don't match? could use ConvertTo, but seems...arbitrary
            // nobody *should* design bytecode set where corresponding elements of stacks from
            // two incoming control flow edges can have different primitive types. objects, sure
            // but not primitive types (even different types of objects should have same primitive
            // type: Address. Expecting to be disappointed here some day...
            assert(_stack[i]->type() == other->_stack[i]->type()); // "invalid stack merge: primitive type mismatch at same depth stack elements");
            cx->MergeDef(PASSLOC, b, other->_stack[i], _stack[i]);
        }
    }
}

//
// VirtualMachineOperandStack API
//

void
VirtualMachineOperandStack::Drop(int32_t depth) {
    assert(_stackTop >= depth-1);
    _stackTop-=depth;
}

void
VirtualMachineOperandStack::Dup() {
    assert(_stackTop >= 0);
    Value *top = _stack[_stackTop];
    Push(top);
}

Value *
VirtualMachineOperandStack::Pick(int32_t depth) {
    assert(_stackTop >= depth);
    return _stack[_stackTop - depth];
}

Value *
VirtualMachineOperandStack::Pop() {
    assert(_stackTop >= 0);
    return _stack[_stackTop--];
}

void
VirtualMachineOperandStack::Push(Value *value) {
    assert(value);
    checkSizeAndGrowIfNeeded();
    _stack[++_stackTop] = value;
}

Value *
VirtualMachineOperandStack::Top() {
    assert(_stackTop >= 0);
    return _stack[_stackTop];
}

// Update the OperandStack_base and _stackTopRegister after the Virtual Machine moves the stack.
// This call will normally be followed by a call to Reload if any of the stack values changed in the move
void
VirtualMachineOperandStack::UpdateStack(LOCATION, Builder *b, Value *stack) {
    fx()->Store(PASSLOC, b, _stackBaseLocal, stack);
}

void
VirtualMachineOperandStack::checkSizeAndGrowIfNeeded() {
    if (_stackTop == _stackMax - 1)
        grow();
}

void
VirtualMachineOperandStack::grow(int32_t growAmount) {
    if (growAmount == 0)
        growAmount = (_stackMax >> 1);

    // if _stackMax == 1, growAmount would still be zero, so bump to 1
    if (growAmount == 0)
        growAmount = 1;

    int32_t newMax = _stackMax + growAmount;
    Value ** newStack = allocator()->allocate<Value *>(newMax);

    int32_t newBytes = newMax * sizeof(Value *);
    memset(newStack, 0, newBytes);

    int32_t numBytes = _stackMax * sizeof(Value *);
    memcpy(newStack, _stack, numBytes);

    if (_stack) {
        allocator()->deallocate(_stack);
    }

    _stack = newStack;
    _stackMax = newMax;
}

void
VirtualMachineOperandStack::init(LOCATION) {
    Base::BaseExtension *bx = this->bx();
    Func::FunctionExtension *fx = this->fx();

    assert(_stack == NULL);
    _stack = allocator()->allocate<Value *>(_stackMax);

    int32_t numBytes = _stackMax * sizeof(Value *);
    memset(_stack, 0, numBytes);

    // Create a unique local symbol to hold this OperandStack's base
    String name(allocator(), "VMOS_StackBase_");
    name.append(String::to_string(allocator(), _id));
    Func::FunctionContext *fc = _comp->context<Func::FunctionContext>();
    _stackBaseLocal = fc->DefineLocal(name, bx->PointerTo(LOC, _elementType));

    // store current operand stack pointer base address so we can use it whenever we need
    // to recreate the stack as the interpreter would have
    Builder *b = _comp->scope<Func::FunctionScope>()->entryPoint<BuilderEntry>()->builder();
    fx->Store(PASSLOC, b, _stackBaseLocal, _stackTopRegister->Load(PASSLOC, b));
}

} // namespace VM
} // namespace JB2
} // namespace OMR
