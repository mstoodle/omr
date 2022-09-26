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

#include "Base/BaseExtension.hpp"
#include "Base/BaseSymbols.hpp"
#include "Base/BaseTypes.hpp"
#include "Base/Function.hpp"
#include "Builder.hpp"
#include "Value.hpp"
#include "VirtualMachineRegister.hpp"
#include "VMExtension.hpp"

namespace OMR {
namespace JitBuilder {
namespace VM {

StateKind VirtualMachineRegister::STATEKIND = KindService::NoKind;
bool VirtualMachineRegister::kindRegistered = false;

const StateKind
VirtualMachineRegister::getStateClassKind() {
    if (!kindRegistered) {
        STATEKIND = VirtualMachineState::kindService.assignKind(KindService::AnyKind, "VirtualMachineRegister");
        kindRegistered = true;
    }
    return STATEKIND;
}

VirtualMachineRegister::VirtualMachineRegister(LOCATION,
                                               VMExtension *vme,
                                               std::string name,
                                               Base::Function *func,
                                               Value * addressOfRegister,
                                               bool doReload)
    : VirtualMachineState(PASSLOC, vme, getStateClassKind())
    , _name(name)
    , _func(func)
    , _addressOfRegister(addressOfRegister)
    , _pRegisterType(addressOfRegister->type()->refine<Base::PointerType>()) {

    // assert that addressOfRegister's type is PointerType
    const Type *regBaseType = _pRegisterType->baseType();
    _integerTypeForAdjustments = regBaseType;
    if (regBaseType->isKind<Base::PointerType>()) {
        _integerTypeForAdjustments = _vme->baseExt()->Word;
        const Type *baseType = regBaseType->refine<Base::PointerType>()->baseType();
        _adjustByStep = baseType->size();
        _isAdjustable = true;
    } else {
        _adjustByStep = 0;
        _isAdjustable = false;
    }
    _local = func->DefineLocal(name, regBaseType);
    if (doReload) {
        for (auto e=0;e < func->numEntryPoints();e++)
            Reload(PASSLOC, func->builderEntry(e));
    }
}

VirtualMachineRegister::VirtualMachineRegister(LOCATION,
                                               VMExtension *vme,
                                               std::string name,
                                               Base::Function * func,
                                               StateKind kind)
        : VirtualMachineState(PASSLOC, vme, kind)
        , _name(name)
        , _func(func)
        , _addressOfRegister(0)
        , _pRegisterType(NULL) {

    }

//
// VirtualMachineState API
//

void
VirtualMachineRegister::Commit(LOCATION, Builder *b) {
    Base::BaseExtension *base = _vme->baseExt();
    Value *oldValue = base->Load(PASSLOC, b, _local);
    base->StoreAt(PASSLOC, b, _addressOfRegister, oldValue);
}

VirtualMachineState *
VirtualMachineRegister::MakeCopy(LOCATION, Builder *b) {
    return new VirtualMachineRegister(PASSLOC, _vme, _name, _func, _addressOfRegister, false);
}

void
VirtualMachineRegister::Reload(LOCATION, Builder *b) {
    Base::BaseExtension *base = _vme->baseExt();
    Value *value = base->LoadAt(PASSLOC, b, _addressOfRegister);
    base->Store(PASSLOC, b, _local, value);
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
    Base::BaseExtension *base = _vme->baseExt();
    Value *oldValue = base->Load(PASSLOC, b, _local);
    Value *newValue = base->IndexAt(PASSLOC, b, oldValue, amount);
    base->Store(PASSLOC, b, _local, newValue);
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
    Base::BaseExtension *base = _vme->baseExt();
    Literal *amountLiteral = base->Word->literal(PASSLOC, b->comp(), reinterpret_cast<LiteralBytes *>(&amount));
    Value *amountValue = base->ConvertTo(LOC, b, _integerTypeForAdjustments, base->Const(PASSLOC, b, amountLiteral));
    Adjust(PASSLOC, b, amountValue);
}

/**
 * @brief used in the compiled method to load the (simulated) register's value
 * @param b the builder where the operation will be placed to load the simulated value
 * @returns TR:IlValue * for the loaded simulated register value
 */
Value *
VirtualMachineRegister::Load(LOCATION, Builder *b) {
    Base::BaseExtension *base = _vme->baseExt();
    return base->Load(PASSLOC, b, _local);
}

/**
 * @brief used in the compiled method to store to the (simulated) register's value
 * @param b the builder where the operation will be placed to store to the simulated value
 */
void
VirtualMachineRegister::Store(LOCATION, Builder *b, Value *value) {
    Base::BaseExtension *base = _vme->baseExt();
    base->Store(PASSLOC, b, _local, value);
}

} // namespace VM
} // namespace JitBuilder
} // namespace OMR
