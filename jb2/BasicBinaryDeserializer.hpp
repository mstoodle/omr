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

#ifndef BASICBINARYDESERIALIZER_INCL
#define BASICBINARYDESERIALIZER_INCL

#include "BinaryDeserializer.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// BasicBinaryDeserializer is the read-side mirror of BasicBinarySerializer.
//
// It decodes raw fixed-width values in native byte order.  Strings are
// decoded as a uint32_t byte-length prefix followed by that many raw bytes.
// All IR identifiers are decoded as raw uint64_t values via readID(IDSpaceID).
//
// Construct with the buffer produced by BasicBinarySerializer:
//   BasicBinaryDeserializer d(ext, serializer.data(), serializer.size());
//

class BasicBinaryDeserializer : public BinaryDeserializer {
    JBALLOC_(BasicBinaryDeserializer)

public:
    DYNAMIC_ALLOC_ONLY(BasicBinaryDeserializer, Extension *ext,
                       const uint8_t *buf, size_t size,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

    // -----------------------------------------------------------------------
    // Primitive read operations — raw fixed-width native byte order
    // -----------------------------------------------------------------------

    virtual void readBool   (bool     & v) override;
    virtual void readInt8   (int8_t   & v) override;
    virtual void readInt16  (int16_t  & v) override;
    virtual void readInt32  (int32_t  & v) override;
    virtual void readInt64  (int64_t  & v) override;
    virtual void readUInt8  (uint8_t  & v) override;
    virtual void readUInt16 (uint16_t & v) override;
    virtual void readUInt32 (uint32_t & v) override;
    virtual void readUInt64 (uint64_t & v) override;
    virtual void readFloat  (float    & v) override;
    virtual void readDouble (double   & v) override;

    // Strings: consume uint32_t byte-length prefix, then that many raw bytes
    virtual void readString(String & s) override;

    // Raw bytes: copy len bytes from the buffer into the caller's buffer
    virtual void readBytes(uint8_t *data, size_t len) override;

    // -----------------------------------------------------------------------
    // IR identifier read — raw uint64_t for every space (matching BasicBinarySerializer)
    // Override readID() in a subclass to apply the inverse of any compact encoding.
    // -----------------------------------------------------------------------

    virtual uint64_t readID(IDSpaceID space) override;

    // -----------------------------------------------------------------------
    // Format-level helpers
    // -----------------------------------------------------------------------

    virtual uint32_t                    readCount()   override;
    virtual std::pair<uint64_t, String> readKindRef() override;

    SUBCLASS_KINDSERVICE_DECL(Extensible, BasicBinaryDeserializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(BASICBINARYDESERIALIZER_INCL)
