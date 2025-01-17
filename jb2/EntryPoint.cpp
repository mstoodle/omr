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
#include "TextLogger.hpp"


namespace OMR {
namespace JB2 {

INIT_JBALLOC(EntryPoint)
SUBCLASS_KINDSERVICE_IMPL(EntryPoint,"EntryPoint",ExtensibleIR,Extensible)

EntryPoint::EntryPoint(Allocator *a, IR *ir, ExtensibleKind kind, EntryID entryID, String name)
    : ExtensibleIR(a, ir->ext(), ir, kind)
    , _id(ir->getEntryPointID())
    , _entryID(entryID)
    , _ir(ir)
    , _name(name) {

}

EntryPoint::EntryPoint(Allocator *a, const EntryPoint *source, IRCloner *cloner)
    : ExtensibleIR(a, source->ext(), cloner->clonedIR(), source->kind())
    , _id(source->_id)
    , _entryID(source->_entryID)
    , _ir(cloner->clonedIR())
    , _name(source->_name) {

}

EntryPoint *
EntryPoint::cloneEntryPoint(Allocator *mem, IRCloner *cloner) const {
    // EntryPoints by default will not be cloned
    return NULL;
}

void
EntryPoint::log(TextLogger & lgr) const {
    lgr.irFlagBegin("entry") << "e" << _id << " " << Extensible::kindService.getName(_kind) << " ";
    logContents(lgr);
    lgr.irFlagEnd();
}

EntryPoint::~EntryPoint() {

}

} // namespace JB2
} // namespace OMR
