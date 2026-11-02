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

#include "Extension.hpp"
#include "ExtensionIRWriter.hpp"
#include "IRSerializer.hpp"
#include "SemanticVersion.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(ExtensionIRWriter, Extension)
SUBCLASS_KINDSERVICE_IMPL(ExtensionIRWriter, "ExtensionIRWriter", Extensible, Extensible)

ExtensionIRWriter::ExtensionIRWriter(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind)
    : Extensible(a, ext, kind)
    , _ext(ext) {
}

ExtensionIRWriter::~ExtensionIRWriter() {
}

void
ExtensionIRWriter::write(IRSerializer & s) {
    s.writeExtID(_ext->id());
    s.writeString(_ext->name());
    const SemanticVersion *v = _ext->semver();
    s.writeUInt16(v->major());
    s.writeUInt16(v->minor());
    s.writeUInt16(v->patch());
    s.writeString(v->preRelease());
    s.writeString(v->buildMetadata());

    writeDeclaredIDSpaces(s);
}

void
ExtensionIRWriter::writeDeclaredIDSpaces(IRSerializer & s) {
    IRSerializer::Section sec(s, "id_spaces");
    s.writeCount(0);
}

void
ExtensionIRWriter::writeElementsForIDSpace(IRSerializer & s, IDSpaceID space) {
    // Default base implementation writes nothing for unhandled ID spaces
}

void
ExtensionIRWriter::writeActions(IRSerializer & s) {
}

} // namespace JB2
} // namespace OMR
