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

#ifndef BINARYSERIALIZER_INCL
#define BINARYSERIALIZER_INCL

#include <cstddef>
#include <cstdint>
#include "IRSerializer.hpp"

namespace OMR {
namespace JB2 {

class Allocator;
class Extension;

//
// BinarySerializer is the abstract mid-layer for binary serialization.
//
// It serializes into a grow-on-demand, Allocator-backed byte buffer.  The
// serialized bytes are accessible via data() / size() after writing, and can
// be copied to any destination (file, socket, shared memory, etc.) by the
// caller.
//
// All write*() methods are still pure virtual: the concrete encoding strategy
// (fixed-width, variable-length, delta-coded, etc.) is left entirely to
// subclasses.  The only shared behaviour at this level is the managed buffer:
// subclasses call append() to push bytes into it.
//
// Section markers write no bytes by default; subclasses may override to emit
// tag bytes or length prefixes if the format requires them.
//
// Concrete subclasses:
//   BasicBinarySerializer      — raw fixed-width encoding (no compression)
//   BinaryStreamSerializer     — delegates buffer flushes to an std::ostream
//

class BinarySerializer : public IRSerializer {
    JBALLOC_(BinarySerializer)

public:
    DYNAMIC_ALLOC_ONLY(BinarySerializer, Extension *ext,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

    // Access the serialized bytes accumulated so far
    const uint8_t * data() const { return _buf; }
    size_t          size() const { return _size; }

    // Section markers: no-op at this level. Override to emit tag bytes.
    virtual void beginSection(const String & name) override { }
    virtual void endSection  (const String & name) override { }

protected:
    // Append len bytes to the managed buffer, growing it as needed.
    // Subclasses call this to emit every encoded value.
    void append(const uint8_t *bytes, size_t len);

    // Convenience: append a single byte
    void appendByte(uint8_t b) { append(&b, 1); }

private:
    static const size_t kInitialCapacity = 256;

    void grow(size_t needed);

    uint8_t * _buf;       // Allocator-owned buffer
    size_t    _size;      // bytes written so far
    size_t    _capacity;  // total bytes allocated

    SUBCLASS_KINDSERVICE_DECL(Extensible, BinarySerializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(BINARYSERIALIZER_INCL)
