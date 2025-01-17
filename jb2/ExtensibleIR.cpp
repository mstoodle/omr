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

#include "AddonIR.hpp"
#include "ExtensibleIR.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC(ExtensibleIR)
SUBCLASS_KINDSERVICE_IMPL(ExtensibleIR, "ExtensibleIR", Extensible, Extensible);

ExtensibleIR::ExtensibleIR(Allocator *a, Extension *ext, IR *ir, KINDTYPE(Extensible) kind)
    : Extensible(a, ext, kind)
    , _ir(ir) {

}

ExtensibleIR::ExtensibleIR(Allocator *a, Extension *ext, Compiler *compiler, KINDTYPE(Extensible) kind)
    : Extensible(a, ext, kind)
    , _ir(compiler->irPrototype()) {
}

ExtensibleIR::ExtensibleIR(Allocator *a, const ExtensibleIR *source, IRCloner *cloner)
    : Extensible(a, source->ext(), source->kind())
    , _ir(cloner->clonedIR()) {

    assert(a == _ir->allocator());
    if (source->addons() != NULL) {
        for (auto it = source->addons()->iterator(); it.hasItem(); it++) {
            Addon *sourceAddon = it.item();
            AddonIR *addon = sourceAddon->refine<AddonIR>();
            AddonIR *clonedAddon = addon->clone(a, cloner);
            this->attach(clonedAddon);
        }
    }
}

ExtensibleIR::~ExtensibleIR() {
}

ExtensibleIR *
ExtensibleIR::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) ExtensibleIR(a, this, cloner);
}

} // namespace JB2
} // namespace OMR
