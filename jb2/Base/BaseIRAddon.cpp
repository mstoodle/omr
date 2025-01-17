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

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/BaseExtension.hpp"
#include "Base/BaseIRAddon.hpp"
#include "Base/BaseTypes.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

INIT_JBALLOC_REUSECAT(BaseIRAddon, Compilation)
SUBCLASS_KINDSERVICE_IMPL(BaseIRAddon,"BaseIRAddon",AddonIR,Extensible)

BaseIRAddon::BaseIRAddon(MEM_LOCATION(a), BaseExtension *bx, IR *root)
    : AddonIR(a, bx, root, KIND(Extensible))
    , Int8(new (a) Int8Type(MEM_PASSLOC(a), bx, root, bx->tInt8))
    , Int16(new (a) Int16Type(MEM_PASSLOC(a), bx, root, bx->tInt16))
    , Int32(new (a) Int32Type(MEM_PASSLOC(a), bx, root, bx->tInt32))
    , Int64(new (a) Int64Type(MEM_PASSLOC(a), bx, root, bx->tInt64))
    , Float32(new (a) Float32Type(MEM_PASSLOC(a), bx, root, bx->tFloat32))
    , Float64(new (a) Float64Type(MEM_PASSLOC(a), bx, root, bx->tFloat64))
    , Address(new (a) AddressType(MEM_PASSLOC(a), bx, root, bx->tAddress))
    , Word(bx->compiler()->platformWordSize() == 64 ? static_cast<const IntegerType *>(this->Int64) : static_cast<const IntegerType *>(this->Int32))
    , _nextCaseID(NoCase+1) {

    notifyCreation(KIND(Extensible));
}

BaseIRAddon::BaseIRAddon(Allocator *a, const BaseIRAddon *source, IRCloner *cloner)
    : AddonIR(a, source, cloner)
    , Int8(cloner->clonedType(source->Int8)->refine<Int8Type>())
    , Int16(cloner->clonedType(source->Int16)->refine<Int16Type>())
    , Int32(cloner->clonedType(source->Int32)->refine<Int32Type>())
    , Int64(cloner->clonedType(source->Int64)->refine<Int64Type>())
    , Float32(cloner->clonedType(source->Float32)->refine<Float32Type>())
    , Float64(cloner->clonedType(source->Float64)->refine<Float64Type>())
    , Address(cloner->clonedType(source->Address)->refine<AddressType>())
    , Word(cloner->clonedType(source->Word)->refine<IntegerType>())
    , _nextCaseID(source->_nextCaseID) {

    notifyCreation(KIND(Extensible));
}

AddonIR *
BaseIRAddon::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) BaseIRAddon(a, this, cloner);
}

const PointerType *
BaseIRAddon::pointerTypeFromBaseType(const Type *baseType) {
    auto found = _pointerTypeFromBaseType.find(baseType);
    if (found != _pointerTypeFromBaseType.end()) {
        const PointerType *t = found->second;
        return t;
    }
    return NULL;
}

void
BaseIRAddon::registerPointerType(const PointerType *pType) {
    const Type *baseType = pType->baseType();
    _pointerTypeFromBaseType.insert({baseType, pType});
}

const StructType *
BaseIRAddon::structTypeFromName(const String & name) {
    auto found = _structTypeFromName.find(name);
    if (found != _structTypeFromName.end()) {
        const StructType *t = found->second;
        return t;
    }
    return NULL;
}

void
BaseIRAddon::registerStructType(const StructType *sType) {
    _structTypeFromName.insert({sType->name(), sType});
}

} // namespace Base
} // namespace JB2
} // namespace OMR
