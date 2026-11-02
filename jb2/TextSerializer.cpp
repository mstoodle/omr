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

#include <iomanip>
#include <sstream>
#include "Extension.hpp"
#include "IDSpaceService.hpp"
#include "TextLogger.hpp"
#include "TextSerializer.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(TextSerializer, Extension)
SUBCLASS_KINDSERVICE_IMPL(TextSerializer, "TextSerializer", IRSerializer, Extensible)

TextSerializer::TextSerializer(Allocator *mem, Extension *ext,
                               TextLogger & logger,
                               KINDTYPE(Extensible) kind)
    : IRSerializer(mem, ext, kind)
    , _logger(logger) {
}

TextSerializer::~TextSerializer() {
}

// ---------------------------------------------------------------------------
// Primitive writes
// ---------------------------------------------------------------------------

void TextSerializer::writeBool(bool v) {
    _logger << (v ? "true" : "false") << " ";
}

void TextSerializer::writeInt8(int8_t v) {
    _logger << static_cast<int32_t>(v) << " ";
}

void TextSerializer::writeInt16(int16_t v) {
    _logger << static_cast<int32_t>(v) << " ";
}

void TextSerializer::writeInt32(int32_t v) {
    _logger << v << " ";
}

void TextSerializer::writeInt64(int64_t v) {
    _logger << v << " ";
}

void TextSerializer::writeUInt8(uint8_t v) {
    _logger << static_cast<uint32_t>(v) << " ";
}

void TextSerializer::writeUInt16(uint16_t v) {
    _logger << static_cast<uint32_t>(v) << " ";
}

void TextSerializer::writeUInt32(uint32_t v) {
    _logger << v << " ";
}

void TextSerializer::writeUInt64(uint64_t v) {
    _logger << v << " ";
}

void TextSerializer::writeFloat(float v) {
    _logger << v << " ";
}

void TextSerializer::writeDouble(double v) {
    _logger << v << " ";
}

void TextSerializer::writeString(const String & s) {
    // Emit as a quoted string to make whitespace visible and parseable.
    _logger << "\"" << s << "\" ";
}

// Raw bytes: emit as a compact hex sequence, e.g. "0a1b2c"
void TextSerializer::writeBytes(const uint8_t *data, size_t len) {
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++)
        hex << std::setw(2) << static_cast<unsigned>(data[i]);
    _logger << hex.str().c_str() << " ";
}

// ---------------------------------------------------------------------------
// ID write — emit the IDSpaceService tag prefix + decimal id, e.g. "t12"
// ---------------------------------------------------------------------------

void TextSerializer::writeID(IDSpaceID space, uint64_t id) {
    String tag = IDSpaceService::service.tagFor(space);
    // Use the first character of the tag as a compact prefix (e.g. 'f' from
    // "fTypeID" → prefix is the lowercase letter after 'f', so "t").
    // Convention: tag is "fXxxID"; prefix character is tag[1] lowercased.
    char prefix = (tag.length() >= 2) ? tag.c_str()[1] : '?';
    _logger << prefix << id << " ";
}

// ---------------------------------------------------------------------------
// Format-level helpers
// ---------------------------------------------------------------------------

void TextSerializer::writeCount(uint32_t count) {
    _logger << count << " ";
}

void TextSerializer::writeKindRef(uint64_t fExtID, const String & kindName) {
    // Emit as "ext:<id> kind:<name>"
    _logger << "ext:" << fExtID << " kind:\"" << kindName << "\" ";
}

// ---------------------------------------------------------------------------
// Section markers
// ---------------------------------------------------------------------------

void TextSerializer::beginSection(const String & name) {
    _logger.sectionStart(name);
}

void TextSerializer::endSection(const String & name) {
    _logger.sectionEnd(name);
}

} // namespace JB2
} // namespace OMR
