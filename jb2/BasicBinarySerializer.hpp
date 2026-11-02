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

#ifndef BASICBINARYSERIALIZER_INCL
#define BASICBINARYSERIALIZER_INCL

#include "BinarySerializer.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// BasicBinarySerializer is the first concrete binary serializer.
//
// It uses raw fixed-width encoding: every value is written in native byte
// order as its exact binary representation.  No compression, no variable-
// length coding, no delta encoding.  This makes the format simple to
// implement and easy to debug, at the cost of size efficiency.
//
// All IR identifiers are written as raw uint64_t via writeID(IDSpaceID, id).
// Future subclasses of BinarySerializer can override writeID() to apply
// compact per-space encodings without changing call sites.
//
// Strings are length-prefixed: a uint32_t byte count followed by the raw
// UTF-8 bytes (no null terminator in the stream).
//

class BasicBinarySerializer : public BinarySerializer {
    JBALLOC_(BasicBinarySerializer)

public:
    DYNAMIC_ALLOC_ONLY(BasicBinarySerializer, Extension *ext,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

    // -----------------------------------------------------------------------
    // Primitive write operations — raw fixed-width native byte order
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

    // Strings: uint32_t byte-length prefix + raw bytes (no null terminator)
    virtual void writeString(const String & s) override;

    // Raw bytes: written as-is
    virtual void writeBytes(const uint8_t *data, size_t len) override;

    // -----------------------------------------------------------------------
    // IR identifier write — raw uint64_t for every space (no compact encoding)
    // Override writeID() in a subclass to apply variable-length or delta coding.
    // -----------------------------------------------------------------------

    virtual void writeID(IDSpaceID space, uint64_t id) override;

    // -----------------------------------------------------------------------
    // Format-level helpers
    // -----------------------------------------------------------------------

    virtual void writeCount  (uint32_t count) override;
    virtual void writeKindRef(uint64_t fExtID, const String & kindName) override;

    SUBCLASS_KINDSERVICE_DECL(Extensible, BasicBinarySerializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(BASICBINARYSERIALIZER_INCL)
