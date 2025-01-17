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
#include "Func/FunctionIRAddon.hpp"
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

INIT_JBALLOC_REUSECAT(FunctionIRAddon, IR)
SUBCLASS_KINDSERVICE_IMPL(FunctionIRAddon,"FunctionIRAddon",AddonIR,Extensible)

FunctionIRAddon::FunctionIRAddon(Allocator *a, FunctionExtension *fx, IR *root)
    : AddonIR(a, fx, root, KIND(Extensible))
    , _functions(NULL, a) {

}

FunctionIRAddon::FunctionIRAddon(Allocator *a, const FunctionIRAddon *source, IRCloner *cloner)
    : AddonIR(a, source->ext()->refine<FunctionExtension>(), cloner->clonedIR(), KIND(Extensible))
    , _functions(NULL, a) {

    // clone functions and functionTypesFromNames

}

AddonIR *
FunctionIRAddon::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) FunctionIRAddon(a, this, cloner);
}

const FunctionType *
FunctionIRAddon::lookupFunctionType(FunctionTypeBuilder & ftb) {
    String name = FunctionType::typeName(root()->refine<IR>()->mem(), ftb);
    auto it = _functionTypesFromName.find(name);
    if (it != _functionTypesFromName.end()) {
        const FunctionType *fType = it->second;
        return fType;
    }
    return NULL;
}

void
FunctionIRAddon::registerFunctionType(const FunctionType *fType) {
    _functionTypesFromName.insert({fType->name(), fType});
}


} // namespace Function
} // namespace JB2
} // namespace OMR
