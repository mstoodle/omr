/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
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

#include "Complex/ComplexIRAddon.hpp"
#include "Complex/ComplexExtension.hpp"
#include "Complex/ComplexTypes.hpp"
#include "Base/Base.hpp"

namespace OMR {
namespace JB2 {
namespace Complex {

INIT_JBALLOC(ComplexIRAddon)
SUBCLASS_KINDSERVICE_IMPL(ComplexIRAddon,"ComplexIRAddon",AddonIR,Extensible);

ComplexIRAddon::ComplexIRAddon(Allocator *a, ComplexExtension *cx, IR *root)
    : AddonIR(a, cx, root, KIND(Extensible))
    , ComplexFloat32(new (a) ComplexFloat32Type(MEM_LOC(a), cx, cx->base()->Float32(root)))
    , ComplexFloat64(new (a) ComplexFloat64Type(MEM_LOC(a), cx, cx->base()->Float64(root))) {

}

ComplexIRAddon::ComplexIRAddon(Allocator *a, const ComplexIRAddon *source, IRCloner *cloner)
    : AddonIR(a, source, cloner)
    , ComplexFloat32(cloner->clonedType(source->ComplexFloat32)->refine<ComplexFloat32Type>())
    , ComplexFloat64(cloner->clonedType(source->ComplexFloat64)->refine<ComplexFloat64Type>()) {

}

AddonIR *
ComplexIRAddon::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) ComplexIRAddon(a, this, cloner);
}

} // namespace Complex
} // namespace JB2
} // namespace OMR

// Made with Bob
