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

#include "NativeEntry.hpp"
#include "TextLogger.hpp"


namespace OMR {
namespace JB2 {

INIT_JBALLOC(NativeEntry)
SUBCLASS_KINDSERVICE_IMPL(NativeEntry,"NativeEntry",EntryPoint,Extensible)

NativeEntry::NativeEntry(Allocator *a, IR *ir, EntryID id, void *entry, String name)
    : EntryPoint(a, ir, KIND(Extensible), id, name)
    , _entry(entry) {

}

// for subclasses only
NativeEntry::NativeEntry(Allocator *a, IR *ir, ExtensibleKind kind, EntryID id, void *entry, String name)
    : EntryPoint(a, ir, kind, id, name)
    , _entry(entry) {

}

NativeEntry::~NativeEntry() {

}

void
NativeEntry::logContents(TextLogger & lgr) const {
    lgr << "entryPC " << (void *)_entry << " ";
}

} // namespace JB2
} // namespace OMR
