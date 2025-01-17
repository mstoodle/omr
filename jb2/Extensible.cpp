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

#include "Compiler.hpp"
#include "CoreExtension.hpp"
#include "Extensible.hpp"
#include "Extension.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC(Extensible)
BASECLASS_KINDSERVICE_IMPL(Extensible);

Extensible::Extensible(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind)
    : Loggable(a)
    , _ext(ext)
    , _compiler(ext->compiler())
    , _addons(NULL)
    , BASECLASS_KINDINIT(kind) {
}

Extensible::~Extensible() {
    if (_addons != NULL) {
        //#if 0 // should be covered already because Addon's are Extensible?
        for (auto it = _addons->iterator(); it.hasItem(); it++) {
            Addon *addon = it.item();
            delete addon;
        }
        //#endif
        delete _addons;
    }
}

void
Extensible::attach(Addon *a) {
    if (_addons == NULL) {
        // use same allocator as for primary (Extensible) object
        Allocator *mem = allocator();
        _addons = new (mem) List<Addon *>(mem, mem);
    }
    #if 0
     else {
        for (auto it = _addons->iterator();it.hasItem();it++) {
            Addon *list_a = it.item();
            if (list_a->kind() == a->kind())
                return;
        }
    }
    #endif
    _addons->push_back(a);
}

void
Extensible::notifyCreation(KINDTYPE(Extensible) kind) {
    compiler()->createAnyAddons(this, kind);
}

} // namespace JB2
} // namespace OMR
