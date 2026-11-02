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

#include <cassert>
#include <cctype>
#include <sstream>
#include <utility>
#include "Extension.hpp"
#include "IDSpaceService.hpp"
#include "TextDeserializer.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(TextDeserializer, Extension)
SUBCLASS_KINDSERVICE_IMPL(TextDeserializer, "TextDeserializer", IRDeserializer, Extensible)

TextDeserializer::TextDeserializer(Allocator *mem, Extension *ext,
                                   std::istream & is,
                                   KINDTYPE(Extensible) kind)
    : IRDeserializer(mem, ext, kind)
    , _is(is) {
}

TextDeserializer::~TextDeserializer() {
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Skip any leading whitespace, then read a single whitespace-delimited token.
static std::string nextToken(std::istream & is) {
    std::string tok;
    is >> tok;
    return tok;
}

// Parse a quoted string token: consume the opening '"', read until closing '"'.
static std::string readQuotedString(std::istream & is) {
    char c;
    // Skip whitespace up to the opening quote.
    while (is.get(c) && std::isspace(static_cast<unsigned char>(c))) { }
    assert(c == '"' && "TextDeserializer: expected opening quote");
    std::string result;
    while (is.get(c) && c != '"')
        result += c;
    return result;
}

// ---------------------------------------------------------------------------
// Primitive reads
// ---------------------------------------------------------------------------

void TextDeserializer::readBool(bool & v) {
    std::string tok = nextToken(_is);
    assert((tok == "true" || tok == "false") && "TextDeserializer: expected 'true' or 'false'");
    v = (tok == "true");
}

void TextDeserializer::readInt8(int8_t & v) {
    int32_t tmp; _is >> tmp; v = static_cast<int8_t>(tmp);
}

void TextDeserializer::readInt16(int16_t & v) {
    int32_t tmp; _is >> tmp; v = static_cast<int16_t>(tmp);
}

void TextDeserializer::readInt32(int32_t & v) {
    _is >> v;
}

void TextDeserializer::readInt64(int64_t & v) {
    _is >> v;
}

void TextDeserializer::readUInt8(uint8_t & v) {
    uint32_t tmp; _is >> tmp; v = static_cast<uint8_t>(tmp);
}

void TextDeserializer::readUInt16(uint16_t & v) {
    uint32_t tmp; _is >> tmp; v = static_cast<uint16_t>(tmp);
}

void TextDeserializer::readUInt32(uint32_t & v) {
    _is >> v;
}

void TextDeserializer::readUInt64(uint64_t & v) {
    _is >> v;
}

void TextDeserializer::readFloat(float & v) {
    _is >> v;
}

void TextDeserializer::readDouble(double & v) {
    _is >> v;
}

void TextDeserializer::readString(String & s) {
    std::string raw = readQuotedString(_is);
    s = String(raw.c_str());
}

// Raw bytes: parse a compact hex sequence (pairs of hex digits), e.g. "0a1b2c"
void TextDeserializer::readBytes(uint8_t *data, size_t len) {
    std::string hex = nextToken(_is);
    assert(hex.length() == len * 2 && "TextDeserializer: hex byte sequence length mismatch");
    for (size_t i = 0; i < len; i++) {
        unsigned byte;
        std::istringstream(hex.substr(i * 2, 2)) >> std::hex >> byte;
        data[i] = static_cast<uint8_t>(byte);
    }
}

// ---------------------------------------------------------------------------
// ID read — consume "<prefix><decimal>" token, validate prefix, return id
// ---------------------------------------------------------------------------

uint64_t TextDeserializer::readID(IDSpaceID space) {
    std::string tok = nextToken(_is);
    assert(!tok.empty() && "TextDeserializer: expected ID token");

    // The expected prefix is tag[1] (the character after 'f' in "fXxxID").
    String tag = IDSpaceService::service.tagFor(space);
    char expected = (tag.length() >= 2) ? tag.c_str()[1] : '?';
    assert(tok[0] == expected && "TextDeserializer: ID prefix mismatch");

    return std::stoull(tok.substr(1));
}

// ---------------------------------------------------------------------------
// Format-level helpers
// ---------------------------------------------------------------------------

uint32_t TextDeserializer::readCount() {
    uint32_t count;
    _is >> count;
    return count;
}

std::pair<uint64_t, String>
TextDeserializer::readKindRef() {
    // Written as "ext:<id> kind:"<name>""
    std::string extTok = nextToken(_is);  // "ext:<id>"
    assert(extTok.substr(0, 4) == "ext:" && "TextDeserializer: expected 'ext:' prefix");
    uint64_t fExtID = std::stoull(extTok.substr(4));

    std::string kindTok = nextToken(_is); // "kind:"
    assert(kindTok == "kind:" && "TextDeserializer: expected 'kind:' token");

    std::string name = readQuotedString(_is);
    return std::make_pair(fExtID, String(name.c_str()));
}

// ---------------------------------------------------------------------------
// Section markers — consume and validate begin/end lines
// ---------------------------------------------------------------------------

void TextDeserializer::beginSection(const String & name) {
    // Consume the section-begin line emitted by TextLogger::sectionStart.
    // TextLogger emits e.g. "[ <name>" on its own line.
    std::string line;
    while (std::getline(_is, line)) {
        // Strip leading whitespace
        size_t pos = line.find_first_not_of(" \t");
        if (pos == std::string::npos) continue;
        std::string trimmed = line.substr(pos);
        if (trimmed.find("[") == 0) {
            // Found the section start marker — verify the name.
            assert(trimmed.find(name.c_str()) != std::string::npos
                   && "TextDeserializer: beginSection name mismatch");
            return;
        }
    }
    assert(false && "TextDeserializer: beginSection marker not found");
}

void TextDeserializer::endSection(const String & name) {
    // Consume the section-end line emitted by TextLogger::sectionEnd.
    // TextLogger emits e.g. "] <name>" on its own line.
    std::string line;
    while (std::getline(_is, line)) {
        size_t pos = line.find_first_not_of(" \t");
        if (pos == std::string::npos) continue;
        std::string trimmed = line.substr(pos);
        if (trimmed.find("]") == 0) {
            assert(trimmed.find(name.c_str()) != std::string::npos
                   && "TextDeserializer: endSection name mismatch");
            return;
        }
    }
    assert(false && "TextDeserializer: endSection marker not found");
}

} // namespace JB2
} // namespace OMR
