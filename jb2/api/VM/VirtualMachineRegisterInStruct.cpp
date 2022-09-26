/*******************************************************************************
 * Copyright (c) 2018, 2022 IBM Corp. and others
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
#include "VirtualMachineRegisterInStruct.hpp"
#include "VMExtension.hpp"


namespace OMR {
namespace JitBuilder {
namespace VM {

StateKind VirtualMachineRegisterInStruct::STATEKIND = KindService::NoKind;
bool VirtualMachineRegisterInStruct::kindRegistered = false;

const StateKind
VirtualMachineRegisterInStruct::getStateClassKind() {
    if (!kindRegistered) {
        STATEKIND = VirtualMachineState::kindService.assignKind(VirtualMachineRegister::getStateClassKind(), "VirtualMachineRegisterInStruct");
	kindRegistered = true;
    }
    return STATEKIND;
}

VirtualMachineRegisterInStruct::VirtualMachineRegisterInStruct(LOCATION,
                                                               VMExtension *vme,
                                                               std::string name,
                                                               Base::Function *func,
                                                               const Base::FieldType * fieldType,
                                                               Base::LocalSymbol * localHoldingStructAddress,
                                                               bool doReload)
    : VirtualMachineRegister(PASSLOC, vme, name, func, getStateClassKind())
    , _localHoldingStructAddress(localHoldingStructAddress)
    , _fieldType(fieldType) {

    const Type *regBaseType = fieldType->type();
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

void
VirtualMachineRegisterInStruct::Commit(LOCATION, Builder *b) {
    Base::BaseExtension *base = _vme->baseExt();
    Value *structBase = base->Load(PASSLOC, b, _localHoldingStructAddress);
    Value *registerValue = base->Load(PASSLOC, b, _local);
    base->StoreFieldAt(PASSLOC, b, _fieldType, structBase, registerValue);
}

VirtualMachineState *
VirtualMachineRegisterInStruct::MakeCopy(LOCATION, Builder *b) {
    return new  VirtualMachineRegisterInStruct(PASSLOC, _vme, _name, _func, _fieldType, _localHoldingStructAddress, false);
}

void
VirtualMachineRegisterInStruct::Reload(LOCATION, Builder *b) {
    Base::BaseExtension *base = _vme->baseExt();
    Value *structBase = base->Load(PASSLOC, b, _localHoldingStructAddress);
    Value *registerValue = base->LoadFieldAt(PASSLOC, b, _fieldType, structBase);
    base->Store(PASSLOC, b, _local, registerValue);
}

} // namespace VM
} // namespace JitBuilder
} // namespace OMR
