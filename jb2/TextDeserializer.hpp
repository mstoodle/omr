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

#ifndef TEXTDESERIALIZER_INCL
#define TEXTDESERIALIZER_INCL

#include <istream>
#include <utility>
#include "IRDeserializer.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// TextDeserializer parses the human-readable text produced by TextSerializer.
//
// Primitive values are parsed from their decimal / string representations.
// IR identifiers are parsed from their type-prefixed form (e.g. "t12", "s7").
// Section markers consume the begin/end lines emitted by TextSerializer and
// assert that the section name matches.
//
// TextDeserializer reads from a caller-supplied std::istream; it does not
// own the stream.
//

class TextDeserializer : public IRDeserializer {
    JBALLOC_(TextDeserializer)

public:
    DYNAMIC_ALLOC_ONLY(TextDeserializer, Extension *ext, std::istream & is,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

    // -----------------------------------------------------------------------
    // Primitive read operations
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
    virtual void readString (String   & s) override;

    // Raw bytes: parse a hex sequence produced by TextSerializer::writeBytes
    virtual void readBytes(uint8_t *data, size_t len) override;

    // -----------------------------------------------------------------------
    // IR identifier read — consume the type prefix, validate against the
    // IDSpaceService tag, and return the numeric ID.
    // -----------------------------------------------------------------------

    virtual uint64_t readID(IDSpaceID space) override;

    // -----------------------------------------------------------------------
    // Format-level helpers
    // -----------------------------------------------------------------------

    virtual uint32_t                    readCount()   override;
    virtual std::pair<uint64_t, String> readKindRef() override;

    // -----------------------------------------------------------------------
    // Section markers — consume and validate begin/end lines
    // -----------------------------------------------------------------------

    virtual void beginSection(const String & name) override;
    virtual void endSection  (const String & name) override;

protected:
    std::istream & _is;

    SUBCLASS_KINDSERVICE_DECL(Extensible, TextDeserializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(TEXTDESERIALIZER_INCL)
