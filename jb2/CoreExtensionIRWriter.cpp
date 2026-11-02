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

#include "CoreExtension.hpp"
#include "CoreExtensionIRWriter.hpp"
#include "IDSpaceService.hpp"
#include "IRSerializer.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(CoreExtensionIRWriter, Extension)
SUBCLASS_KINDSERVICE_IMPL(CoreExtensionIRWriter, "CoreExtensionIRWriter", ExtensionIRWriter, Extensible)

CoreExtensionIRWriter::CoreExtensionIRWriter(Allocator *a, CoreExtension *ext)
    : ExtensionIRWriter(a, ext, CLASSKIND(CoreExtensionIRWriter, Extensible))
    , _spExtID       (IDSpaceService::service.idSpaceFor("fExtID"))
    , _spTypeID      (IDSpaceService::service.idSpaceFor("fTypeID"))
    , _spActionID    (IDSpaceService::service.idSpaceFor("fActionID"))
    , _spLiteralID   (IDSpaceService::service.idSpaceFor("fLiteralID"))
    , _spSymbolID    (IDSpaceService::service.idSpaceFor("fSymbolID"))
    , _spEntryPointID(IDSpaceService::service.idSpaceFor("fEntryPointID"))
    , _spContextID   (IDSpaceService::service.idSpaceFor("fContextID"))
    , _spScopeID     (IDSpaceService::service.idSpaceFor("fScopeID"))
    , _spBuilderID   (IDSpaceService::service.idSpaceFor("fBuilderID"))
    , _spValueID     (IDSpaceService::service.idSpaceFor("fValueID"))
    , _spOperationID (IDSpaceService::service.idSpaceFor("fOperationID")) {
}

CoreExtensionIRWriter::~CoreExtensionIRWriter() {
}

void
CoreExtensionIRWriter::writeAction(IRSerializer & s, ActionID action, const String & name) {
    s.writeActionID(action);
    s.writeString(name);
}

void
CoreExtensionIRWriter::writeAction(IRSerializer & s, ActionID action) {
    s.writeActionID(action);
    s.writeString(_ext->actionName(action));
}

void
CoreExtensionIRWriter::writeDeclaredIDSpaces(IRSerializer & s) {
    IRSerializer::Section sec(s, "id_spaces");
    static const char * const standardSpaces[] = {
        "fTypeID",
        "fActionID",
        "fLiteralID",
        "fSymbolID",
        "fEntryPointID",
        "fContextID",
        "fScopeID",
        "fBuilderID",
        "fValueID",
        "fOperationID"
    };
    uint32_t count = sizeof(standardSpaces) / sizeof(standardSpaces[0]);
    s.writeCount(count);
    for (uint32_t i = 0; i < count; ++i) {
        s.writeString(String(allocator(), standardSpaces[i]));
    }
}

void
CoreExtensionIRWriter::writeElementsForIDSpace(IRSerializer & s, IDSpaceID space) {
    if (isActionIDSpace(space)) {
        IRSerializer::Section sec(s, "actions");
        CoreExtension *core = static_cast<CoreExtension *>(_ext);
        s.writeCount(2);
        writeAction(s, core->aAppendBuilder);
        writeAction(s, core->aMergeDef);
    }
}

} // namespace JB2
} // namespace OMR
