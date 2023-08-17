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

#include "Compilation.hpp"
#include "EntryPoint.hpp"
#include "IRCloner.hpp"


namespace OMR {
namespace JitBuilder {

INIT_JBALLOC(EntryPoint)
BASECLASS_KINDSERVICE_IMPL(EntryPoint)

EntryPoint::EntryPoint(Allocator *a, IR *ir, EntryPointKind kind, EntryID entryID, String name)
    : Allocatable(a)
    , _id(ir->getEntryPointID())
    , _entryID(entryID)
    , _ir(ir)
    , _name(name)
    , BASECLASS_KINDINIT(kind) {

}

EntryPoint::EntryPoint(Allocator *a, const EntryPoint *source, IRCloner *cloner)
    : Allocatable(a)
    , _id(source->_id)
    , _entryID(source->_entryID)
    , _ir(cloner->clonedIR())
    , _name(source->_name)
    , BASECLASS_KINDINIT(source->_kind) {

}

EntryPoint *
EntryPoint::clone(Allocator *mem, IRCloner *cloner) const {
    // EntryPoints by default will not be cloned
    return NULL;
}

EntryPoint::~EntryPoint() {

}

} // namespace JitBuilder
} // namespace OMR
