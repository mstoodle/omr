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

#ifndef TEXTSERIALIZER_INCL
#define TEXTSERIALIZER_INCL

#include "IRSerializer.hpp"
#include "TextLogger.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// TextSerializer produces human-readable text output by delegating all
// formatting to an existing TextLogger.
//
// Primitive values are emitted as their decimal / string representations.
// IR identifiers are emitted with a type prefix derived from the IDSpaceService
// tag for the space (e.g. "t12" for a TypeID, "s7" for a SymbolID) matching
// the conventions already used by TextLogger.
// Section markers emit indented begin/end lines matching the existing IR log
// format so TextSerializer output is directly comparable to TextLogger output.
//
// TextSerializer does not own the TextLogger; the caller must ensure the
// logger outlives the serializer.
//

class TextSerializer : public IRSerializer {
    JBALLOC_(TextSerializer)

public:
    DYNAMIC_ALLOC_ONLY(TextSerializer, Extension *ext, TextLogger & logger,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

    TextLogger & logger() const { return _logger; }

    // -----------------------------------------------------------------------
    // Primitive write operations
    // -----------------------------------------------------------------------

    virtual void writeBool   (bool     v) override;
    virtual void writeInt8   (int8_t   v) override;
    virtual void writeInt16  (int16_t  v) override;
    virtual void writeInt32  (int32_t  v) override;
    virtual void writeInt64  (int64_t  v) override;
    virtual void writeUInt8  (uint8_t  v) override;
    virtual void writeUInt16 (uint16_t v) override;
    virtual void writeUInt32 (uint32_t v) override;
    virtual void writeUInt64 (uint64_t v) override;
    virtual void writeFloat  (float    v) override;
    virtual void writeDouble (double   v) override;
    virtual void writeString (const String & s) override;

    // Raw bytes: emitted as a hex sequence, e.g. "0x0a1b2c..."
    virtual void writeBytes(const uint8_t *data, size_t len) override;

    // -----------------------------------------------------------------------
    // IR identifier write — emits "<prefix><id>", e.g. "t12", "s7", "b3".
    // The prefix is derived from the IDSpaceService tag for the space.
    // -----------------------------------------------------------------------

    virtual void writeID(IDSpaceID space, uint64_t id) override;

    // -----------------------------------------------------------------------
    // Format-level helpers
    // -----------------------------------------------------------------------

    virtual void writeCount  (uint32_t count) override;
    virtual void writeKindRef(uint64_t fExtID, const String & kindName) override;

    // -----------------------------------------------------------------------
    // Section markers — delegate to TextLogger indent/section helpers
    // -----------------------------------------------------------------------

    virtual void beginSection(const String & name) override;
    virtual void endSection  (const String & name) override;

protected:
    TextLogger & _logger;

    SUBCLASS_KINDSERVICE_DECL(Extensible, TextSerializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(TEXTSERIALIZER_INCL)
