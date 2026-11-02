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

#include "JBIRWriter.hpp"

namespace OMR {
namespace JB2 {

// ---------------------------------------------------------------------------
// JBIRFileHeader::current()
// Defined here because it calls JBIR::hostLittleEndian(), which lives in
// JBIRWriter.hpp. JBIRFileHeader.hpp deliberately has no such dependency.
// ---------------------------------------------------------------------------

JBIRFileHeader
JBIRFileHeader::current() {
    JBIRFileHeader h;
    h.magic[0]     = JBIR_MAGIC[0];
    h.magic[1]     = JBIR_MAGIC[1];
    h.magic[2]     = JBIR_MAGIC[2];
    h.magic[3]     = JBIR_MAGIC[3];
    h.flags0       = JBIR::hostLittleEndian() ? JBIR_FLAG0_LITTLE_ENDIAN : 0u;
    h.flags1       = 0u;
    h.versionMajor = JBIR_VERSION_MAJOR;
    h.versionMinor = JBIR_VERSION_MINOR;
    h.versionPatch = JBIR_VERSION_PATCH;
    return h;
}

// ---------------------------------------------------------------------------
// JBIRWriter
// ---------------------------------------------------------------------------

JBIRWriter::JBIRWriter()
    : _fileLittleEndian(hostLittleEndian()) {
}

bool
JBIRWriter::writeHeader(const JBIRFileHeader &h) {
    // Record the file's byte order so subsequent writeUInt*() calls use it.
    _fileLittleEndian = h.isLittleEndian();

    // magic: 4 raw bytes, never byte-swapped
    if (!writeRawBytes(h.magic, 4))
        return false;

    // flags0 and flags1: single bytes, never byte-swapped
    uint8_t fb = h.flags0;
    if (!writeRawBytes(&fb, 1))
        return false;
    fb = h.flags1;
    if (!writeRawBytes(&fb, 1))
        return false;

    // version fields: uint16 in the file's byte order
    if (!writeUInt16(h.versionMajor))
        return false;
    if (!writeUInt16(h.versionMinor))
        return false;
    if (!writeUInt16(h.versionPatch))
        return false;

    return true;
}

bool
JBIRWriter::writeUInt16(uint16_t v) {
    uint16_t w = fixEndian16(v, _fileLittleEndian);
    return writeRawBytes(reinterpret_cast<const uint8_t *>(&w), sizeof(w));
}

bool
JBIRWriter::writeUInt32(uint32_t v) {
    uint32_t w = fixEndian32(v, _fileLittleEndian);
    return writeRawBytes(reinterpret_cast<const uint8_t *>(&w), sizeof(w));
}

bool
JBIRWriter::writeUInt64(uint64_t v) {
    uint64_t w = fixEndian64(v, _fileLittleEndian);
    return writeRawBytes(reinterpret_cast<const uint8_t *>(&w), sizeof(w));
}

} // namespace JB2
} // namespace OMR
