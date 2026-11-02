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

#ifndef BINARYDESERIALIZER_INCL
#define BINARYDESERIALIZER_INCL

#include <cstddef>
#include <cstdint>
#include "IRDeserializer.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// BinaryDeserializer is the abstract mid-layer for binary deserialization.
// It is the read-side mirror of BinarySerializer.
//
// It reads from a caller-supplied byte buffer (const uint8_t*, size_t).  The
// buffer is not owned by this class; the caller is responsible for keeping it
// live for the lifetime of the deserializer.  This matches the typical usage
// where BinarySerializer::data() / size() are passed directly here.
//
// All read*() methods are still pure virtual: the concrete decoding strategy
// is left entirely to subclasses.  Subclasses call consume() to advance the
// read cursor and retrieve the next n bytes from the buffer.
//
// Section markers consume no bytes by default; subclasses may override to
// consume and validate tag bytes if the format emits them.
//
// Concrete subclasses:
//   BasicBinaryDeserializer      — raw fixed-width decoding (matches BasicBinarySerializer)
//   BinaryStreamDeserializer     — reads from an std::istream instead of a buffer
//

class BinaryDeserializer : public IRDeserializer {
    JBALLOC_(BinaryDeserializer)

public:
    DYNAMIC_ALLOC_ONLY(BinaryDeserializer, Extension *ext,
                       const uint8_t *buf, size_t size,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

    // True when all bytes in the buffer have been consumed
    bool atEnd() const { return _cursor >= _end; }

    // Section markers: no-op at this level. Override to consume tag bytes.
    virtual void beginSection(const String & name) override { }
    virtual void endSection  (const String & name) override { }

protected:
    // Consume len bytes from the buffer and return a pointer to the first.
    // Advances the internal cursor; asserts if the buffer would be overrun.
    const uint8_t * consume(size_t len);

    // Convenience: consume a single byte
    uint8_t consumeByte() { return *consume(1); }

private:
    const uint8_t * _cursor;  // next byte to read
    const uint8_t * _end;     // one past the last byte

    SUBCLASS_KINDSERVICE_DECL(Extensible, BinaryDeserializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(BINARYDESERIALIZER_INCL)
