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

#include <cstring>
#include "BasicBinarySerializer.hpp"
#include "Extension.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(BasicBinarySerializer, Extension)
SUBCLASS_KINDSERVICE_IMPL(BasicBinarySerializer, "BasicBinarySerializer", BinarySerializer, Extensible)

BasicBinarySerializer::BasicBinarySerializer(Allocator *mem, Extension *ext,
                                             KINDTYPE(Extensible) kind)
    : BinarySerializer(mem, ext, kind) {
}

BasicBinarySerializer::~BasicBinarySerializer() {
}

// ---------------------------------------------------------------------------
// Primitive writes — raw fixed-width native byte order
// ---------------------------------------------------------------------------

void BasicBinarySerializer::writeBool(bool v) {
    uint8_t b = v ? 1 : 0;
    append(&b, 1);
}

void BasicBinarySerializer::writeInt8(int8_t v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeInt16(int16_t v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeInt32(int32_t v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeInt64(int64_t v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeUInt8(uint8_t v) {
    append(&v, 1);
}

void BasicBinarySerializer::writeUInt16(uint16_t v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeUInt32(uint32_t v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeUInt64(uint64_t v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeFloat(float v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

void BasicBinarySerializer::writeDouble(double v) {
    append(reinterpret_cast<const uint8_t *>(&v), sizeof(v));
}

// Strings: uint32_t byte count + raw bytes (no null terminator)
void BasicBinarySerializer::writeString(const String & s) {
    uint32_t len = static_cast<uint32_t>(s.length());
    writeUInt32(len);
    append(reinterpret_cast<const uint8_t *>(s.c_str()), len);
}

void BasicBinarySerializer::writeBytes(const uint8_t *data, size_t len) {
    append(data, len);
}

// ---------------------------------------------------------------------------
// ID write — raw uint64_t regardless of space
// ---------------------------------------------------------------------------

void BasicBinarySerializer::writeID(IDSpaceID /*space*/, uint64_t id) {
    append(reinterpret_cast<const uint8_t *>(&id), sizeof(id));
}

// ---------------------------------------------------------------------------
// Format-level helpers
// ---------------------------------------------------------------------------

void BasicBinarySerializer::writeCount(uint32_t count) {
    writeUInt32(count);
}

void BasicBinarySerializer::writeKindRef(uint64_t fExtID, const String & kindName) {
    writeUInt64(fExtID);
    writeString(kindName);
}

} // namespace JB2
} // namespace OMR
