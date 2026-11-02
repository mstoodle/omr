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
#include <utility>
#include "BasicBinaryDeserializer.hpp"
#include "Extension.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(BasicBinaryDeserializer, Extension)
SUBCLASS_KINDSERVICE_IMPL(BasicBinaryDeserializer, "BasicBinaryDeserializer", BinaryDeserializer, Extensible)

BasicBinaryDeserializer::BasicBinaryDeserializer(Allocator *mem, Extension *ext,
                                                 const uint8_t *buf, size_t size,
                                                 KINDTYPE(Extensible) kind)
    : BinaryDeserializer(mem, ext, buf, size, kind) {
}

BasicBinaryDeserializer::~BasicBinaryDeserializer() {
}

// ---------------------------------------------------------------------------
// Primitive reads — raw fixed-width native byte order
// ---------------------------------------------------------------------------

void BasicBinaryDeserializer::readBool(bool & v) {
    v = (*consume(1) != 0);
}

void BasicBinaryDeserializer::readInt8(int8_t & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readInt16(int16_t & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readInt32(int32_t & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readInt64(int64_t & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readUInt8(uint8_t & v) {
    v = *consume(1);
}

void BasicBinaryDeserializer::readUInt16(uint16_t & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readUInt32(uint32_t & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readUInt64(uint64_t & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readFloat(float & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

void BasicBinaryDeserializer::readDouble(double & v) {
    memcpy(&v, consume(sizeof(v)), sizeof(v));
}

// Strings: consume uint32_t byte count, then that many raw bytes
void BasicBinaryDeserializer::readString(String & s) {
    uint32_t len;
    readUInt32(len);
    // Construct the string from the raw bytes in the buffer.
    const char *p = reinterpret_cast<const char *>(consume(len));
    // Build a null-terminated temporary so String can copy it.
    // String's c_str() contract requires null termination; we can't hand a
    // non-terminated pointer directly to a String constructor.
    char *tmp = static_cast<char *>(allocator()->allocate(len + 1, NoAllocationCategory));
    memcpy(tmp, p, len);
    tmp[len] = '\0';
    s = String(tmp);
    allocator()->deallocate(tmp);
}

void BasicBinaryDeserializer::readBytes(uint8_t *data, size_t len) {
    memcpy(data, consume(len), len);
}

// ---------------------------------------------------------------------------
// ID read — raw uint64_t regardless of space
// ---------------------------------------------------------------------------

uint64_t BasicBinaryDeserializer::readID(IDSpaceID /*space*/) {
    uint64_t id;
    memcpy(&id, consume(sizeof(id)), sizeof(id));
    return id;
}

// ---------------------------------------------------------------------------
// Format-level helpers
// ---------------------------------------------------------------------------

uint32_t BasicBinaryDeserializer::readCount() {
    uint32_t count;
    readUInt32(count);
    return count;
}

std::pair<uint64_t, String>
BasicBinaryDeserializer::readKindRef() {
    uint64_t fExtID;
    readUInt64(fExtID);
    String name;
    readString(name);
    return std::make_pair(fExtID, name);
}

} // namespace JB2
} // namespace OMR
