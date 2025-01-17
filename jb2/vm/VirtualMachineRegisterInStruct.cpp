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

#include "JBCore.hpp"
#include "VirtualMachineRegisterInStruct.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_REUSECAT(VirtualMachineRegisterInStruct, VirtualMachineState)
SUBCLASS_KINDSERVICE_IMPL(VirtualMachineRegisterInStruct,"VirtualMachineRegisterInStruct",VirtualMachineRegister,Extensible);


VirtualMachineRegisterInStruct::VirtualMachineRegisterInStruct(MEM_LOCATION(a),
                                                               VMExtension *vmx,
                                                               String name,
                                                               Compilation *comp,
                                                               const Base::FieldType * fieldType,
                                                               Func::LocalSymbol * localHoldingStructAddress,
                                                               bool doReload)
    : VirtualMachineRegister(MEM_PASSLOC(a), vmx, name, comp, CLASSKIND(VirtualMachineRegisterInStruct,Extensible))
    , _localHoldingStructAddress(localHoldingStructAddress)
    , _fieldType(fieldType) {

    const Type *regBaseType = fieldType->type();
    _integerTypeForAdjustments = regBaseType;
    if (regBaseType->isKind<Base::PointerType>()) {
        _integerTypeForAdjustments = bx()->Word(comp->ir());
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

VirtualMachineRegisterInStruct::VirtualMachineRegisterInStruct(Allocator *a, const VirtualMachineRegisterInStruct *source, IRCloner *cloner)
    : VirtualMachineRegister(a, source, cloner)
    , _localHoldingStructAddress(cloner->clonedSymbol(source->_localHoldingStructAddress)->refine<Func::LocalSymbol>())
    , _fieldType(cloner->clonedType(source->_fieldType)->refine<Base::FieldType>()) {

}

VirtualMachineState *
VirtualMachineRegisterInStruct::clone(Allocator *mem, IRCloner *cloner) const {
    return new (mem) VirtualMachineRegisterInStruct(mem, this, cloner);
}

void
VirtualMachineRegisterInStruct::Commit(LOCATION, Builder *b) {
    Base::BaseExtension *bx = this->bx();
    Func::FunctionExtension *fx = this->fx();
    Value *structBase = fx->Load(PASSLOC, b, _localHoldingStructAddress);
    Value *registerValue = fx->Load(PASSLOC, b, _local);
    bx->StoreFieldAt(PASSLOC, b, _fieldType, structBase, registerValue);
}

VirtualMachineState *
VirtualMachineRegisterInStruct::MakeCopy(LOCATION, Builder *b) {
    Allocator *mem = allocator();
    return new (mem) VirtualMachineRegisterInStruct(MEM_PASSLOC(mem), vmx(), _name, _comp, _fieldType, _localHoldingStructAddress, false);
}

void
VirtualMachineRegisterInStruct::Reload(LOCATION, Builder *b) {
    Base::BaseExtension *bx = this->bx();
    Func::FunctionExtension *fx = this->fx();
    Value *structBase = fx->Load(PASSLOC, b, _localHoldingStructAddress);
    Value *registerValue = bx->LoadFieldAt(PASSLOC, b, _fieldType, structBase);
    fx->Store(PASSLOC, b, _local, registerValue);
}

} // namespace VM
} // namespace JB2
} // namespace OMR
