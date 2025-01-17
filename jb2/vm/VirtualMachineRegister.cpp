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

#include "vm/VirtualMachineRegister.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_REUSECAT(VirtualMachineRegister, VirtualMachineState);
SUBCLASS_KINDSERVICE_IMPL(VirtualMachineRegister,"VirtualMachineRegister",VirtualMachineState,Extensible);

VirtualMachineRegister::VirtualMachineRegister(MEM_LOCATION(a),
                                               VMExtension *vmx,
                                               String name,
                                               Compilation *comp,
                                               Value * addressOfRegister,
                                               bool doReload)
    : VirtualMachineState(MEM_PASSLOC(a), vmx, CLASSKIND(VirtualMachineRegister,Extensible))
    , _name(name)
    , _comp(comp)
    , _addressOfRegister(addressOfRegister)
    , _pRegisterType(addressOfRegister->type()->refine<Base::PointerType>()) {

    // assert that addressOfRegister's type is PointerType
    const Type *regBaseType = _pRegisterType->baseType();
    _integerTypeForAdjustments = regBaseType;
    if (regBaseType->isKind<Base::PointerType>()) {
        _integerTypeForAdjustments = vmx->bx()->Word(comp->ir());
        const Type *baseType = regBaseType->refine<Base::PointerType>()->baseType();
        _adjustByStep = baseType->size();
        _isAdjustable = true;
    } else {
        _adjustByStep = 0;
        _isAdjustable = false;
    }
    Func::FunctionContext *fc = comp->context<Func::FunctionContext>();
    _local = fc->DefineLocal(name, regBaseType);
    if (doReload) {
        for (auto e=0;e < comp->scope<Scope>()->numEntryPoints<BuilderEntry>();e++)
            Reload(PASSLOC, comp->scope<Scope>()->entryPoint<BuilderEntry>(e)->builder());
    }
}

VirtualMachineRegister::VirtualMachineRegister(MEM_LOCATION(a),
                                               VMExtension *vmx,
                                               String name,
                                               Compilation * comp,
                                               KINDTYPE(Extensible) kind)
        : VirtualMachineState(MEM_PASSLOC(a), vmx, kind)
        , _name(name)
        , _comp(comp)
        , _addressOfRegister(0)
        , _pRegisterType(NULL) {

    }

VirtualMachineRegister::VirtualMachineRegister(Allocator *a, const VirtualMachineRegister *source, IRCloner *cloner)
        : VirtualMachineState(a, source, cloner)
        , _name(source->_name)
        , _comp(source->_comp)
        , _addressOfRegister(cloner->clonedValue(source->_addressOfRegister))
        , _pRegisterType(cloner->clonedType(source->_pRegisterType)->refine<Base::PointerType>()) {

    }

VirtualMachineState *
VirtualMachineRegister::clone(Allocator *mem, IRCloner *cloner) const {
    return new (mem) VirtualMachineRegister(mem, this, cloner);
}

//
// VirtualMachineState API
//

void
VirtualMachineRegister::Commit(LOCATION, Builder *b) {
    Base::BaseExtension *bx = vmx()->bx();
    Func::FunctionExtension *fx = vmx()->fx();
    Value *oldValue = fx->Load(PASSLOC, b, _local);
    bx->StoreAt(PASSLOC, b, _addressOfRegister, oldValue);
}

VirtualMachineState *
VirtualMachineRegister::MakeCopy(LOCATION, Builder *b) {
    Allocator *mem = allocator();
    return new (mem) VirtualMachineRegister(MEM_PASSLOC(mem), vmx(), _name, _comp, _addressOfRegister, false);
}

void
VirtualMachineRegister::Reload(LOCATION, Builder *b) {
    Base::BaseExtension *bx = vmx()->bx();
    Func::FunctionExtension *fx = vmx()->fx();
    Value *value = bx->LoadAt(PASSLOC, b, _addressOfRegister);
    fx->Store(PASSLOC, b, _local, value);
}

//
// VirtualMachineRegister API
//

/**
 * @brief used in the compiled method to add to the (simulated) register's value
 * @param b the builder where the operation will be placed to add to the simulated value
 * @param amount the TR::IlValue that should be added to the simulated value, after multiplication by _adjustByStep
 * Adjust() is really a convenience function for the common operation of adding a value to the register. More
 * complicated operations (e.g. multiplying the value) can be built using Load() and Store() if needed.
 */
void
VirtualMachineRegister::Adjust(LOCATION, Builder *b, Value *amount) {
    Base::BaseExtension *bx = vmx()->bx();
    Func::FunctionExtension *fx = vmx()->fx();
    Value *oldValue = fx->Load(PASSLOC, b, _local);
    Value *newValue = bx->IndexAt(PASSLOC, b, oldValue, amount);
    fx->Store(PASSLOC, b, _local, newValue);
}

/**
 * @brief used in the compiled method to add to the (simulated) register's value
 * @param b the builder where the operation will be placed to add to the simulated value
 * @param amount the constant value that should be added to the simulated value, after multiplication by _adjustByStep
 * Adjust() is really a convenience function for the common operation of adding a value to the register. More
 * complicated operations (e.g. multiplying the value) can be built using Load() and Store() if needed.
 */
void
VirtualMachineRegister::Adjust(LOCATION, Builder *b, size_t amount) {
    Base::BaseExtension *bx = this->bx();
    IR *ir = b->ir();
    Literal *amountLiteral = bx->Word(ir)->literal(PASSLOC, reinterpret_cast<LiteralBytes *>(&amount));
    Value *amountValue = bx->ConvertTo(LOC, b, _integerTypeForAdjustments, bx->Const(PASSLOC, b, amountLiteral));
    Adjust(PASSLOC, b, amountValue);
}

/**
 * @brief used in the compiled method to load the (simulated) register's value
 * @param b the builder where the operation will be placed to load the simulated value
 * @returns TR:IlValue * for the loaded simulated register value
 */
Value *
VirtualMachineRegister::Load(LOCATION, Builder *b) {
    Func::FunctionExtension *fx = vmx()->fx();
    return fx->Load(PASSLOC, b, _local);
}

/**
 * @brief used in the compiled method to store to the (simulated) register's value
 * @param b the builder where the operation will be placed to store to the simulated value
 */
void
VirtualMachineRegister::Store(LOCATION, Builder *b, Value *value) {
    Func::FunctionExtension *fx = vmx()->fx();
    fx->Store(PASSLOC, b, _local, value);
}

} // namespace VM
} // namespace JB2
} // namespace OMR
