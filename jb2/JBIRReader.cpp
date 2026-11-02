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

#include "JBIRReader.hpp"

namespace OMR {
namespace JB2 {

JBIRReader::JBIRReader()
    : _fileLittleEndian(hostLittleEndian()) {
}

bool
JBIRReader::readHeader(JBIRFileHeader &h) {
    // magic: 4 raw bytes
    if (!readRawBytes(h.magic, 4))
        return false;
    if (h.magic[0] != JBIR_MAGIC[0] ||
        h.magic[1] != JBIR_MAGIC[1] ||
        h.magic[2] != JBIR_MAGIC[2] ||
        h.magic[3] != JBIR_MAGIC[3])
        return false;

    // flags0: bit 0 is the endianness flag; bits 1-7 must be zero
    if (!readRawBytes(&h.flags0, 1))
        return false;
    if (h.flags0 & ~JBIR_FLAG0_LITTLE_ENDIAN)
        return false;

    // flags1: reserved, must be zero
    if (!readRawBytes(&h.flags1, 1))
        return false;
    if (h.flags1 != 0u)
        return false;

    // Now we know the file's byte order: record it for subsequent reads.
    _fileLittleEndian = h.isLittleEndian();

    // version fields: uint16 in the file's byte order
    if (!readUInt16(h.versionMajor))
        return false;
    if (!readUInt16(h.versionMinor))
        return false;
    if (!readUInt16(h.versionPatch))
        return false;

    return true;
}

bool
JBIRReader::readUInt16(uint16_t &v) {
    uint16_t raw;
    if (!readRawBytes(reinterpret_cast<uint8_t *>(&raw), sizeof(raw)))
        return false;
    v = fixEndian16(raw, _fileLittleEndian);
    return true;
}

bool
JBIRReader::readUInt32(uint32_t &v) {
    uint32_t raw;
    if (!readRawBytes(reinterpret_cast<uint8_t *>(&raw), sizeof(raw)))
        return false;
    v = fixEndian32(raw, _fileLittleEndian);
    return true;
}

bool
JBIRReader::readUInt64(uint64_t &v) {
    uint64_t raw;
    if (!readRawBytes(reinterpret_cast<uint8_t *>(&raw), sizeof(raw)))
        return false;
    v = fixEndian64(raw, _fileLittleEndian);
    return true;
}

} // namespace JB2
} // namespace OMR
