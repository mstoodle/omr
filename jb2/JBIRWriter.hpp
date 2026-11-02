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

// JBIRWriter.hpp — JBIR base class, abstract writer, and shared endianness machinery.
//
// JBIR          — base class holding platform endianness detection and the
//                 byte-swap helpers. JBIRWriter and JBIRReader both inherit
//                 from it.
//
// JBIRWriter    — abstract base class for serialization writers. Subclasses
//                 provide an implementation of writeRawBytes().

#ifndef JBIRWRITER_INCL
#define JBIRWRITER_INCL

#include <cstddef>
#include <cstdint>
#include "JBIRFileHeader.hpp"

namespace OMR {
namespace JB2 {

// ---------------------------------------------------------------------------
// JBIR — common base for JBIRWriter and JBIRReader
//
// Provides:
//   hostLittleEndian()  — true when the current platform is little-endian
//   swap16/32/64()      — unconditional byte reversal
//   fixEndian16/32/64() — swap value if file byte order differs from host
// ---------------------------------------------------------------------------

class JBIR {
public:
    // True when the host platform is little-endian.
    static bool hostLittleEndian() {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
        return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#else
        const uint16_t probe = 1u;
        const uint8_t *p = reinterpret_cast<const uint8_t *>(&probe);
        return p[0] == 1u;
#endif
    }

protected:
    static uint16_t swap16(uint16_t v) {
        return static_cast<uint16_t>((v >> 8) | (v << 8));
    }

    static uint32_t swap32(uint32_t v) {
        return ((v & 0x000000FFu) << 24)
             | ((v & 0x0000FF00u) <<  8)
             | ((v & 0x00FF0000u) >>  8)
             | ((v & 0xFF000000u) >> 24);
    }

    static uint64_t swap64(uint64_t v) {
        return ((v & UINT64_C(0x00000000000000FF)) << 56)
             | ((v & UINT64_C(0x000000000000FF00)) << 40)
             | ((v & UINT64_C(0x0000000000FF0000)) << 24)
             | ((v & UINT64_C(0x00000000FF000000)) <<  8)
             | ((v & UINT64_C(0x000000FF00000000)) >>  8)
             | ((v & UINT64_C(0x0000FF0000000000)) >> 24)
             | ((v & UINT64_C(0x00FF000000000000)) >> 40)
             | ((v & UINT64_C(0xFF00000000000000)) >> 56);
    }

    // Convert a host-order value to the file's byte order (or back).
    // fileLittleEndian comes from the flags0 byte in the header.
    // Both directions are the same operation (swap is its own inverse).
    static uint16_t fixEndian16(uint16_t v, bool fileLittleEndian) {
        return (fileLittleEndian != hostLittleEndian()) ? swap16(v) : v;
    }
    static uint32_t fixEndian32(uint32_t v, bool fileLittleEndian) {
        return (fileLittleEndian != hostLittleEndian()) ? swap32(v) : v;
    }
    static uint64_t fixEndian64(uint64_t v, bool fileLittleEndian) {
        return (fileLittleEndian != hostLittleEndian()) ? swap64(v) : v;
    }
};

// ---------------------------------------------------------------------------
// JBIRWriter — abstract base class for JBIR writers
// ---------------------------------------------------------------------------

class JBIRWriter : public JBIR {
public:
    virtual ~JBIRWriter() = default;

    // Write the JBIR file header.
    // Returns true on success, false if any write fails.
    // Must be called before any writeUInt*() call.
    bool writeHeader(const JBIRFileHeader &h);

    // Write integers in the byte order recorded in the last written header.
    // Must be called after writeHeader().
    bool writeUInt16(uint16_t v);
    bool writeUInt32(uint32_t v);
    bool writeUInt64(uint64_t v);

protected:
    JBIRWriter();

    // Pure virtual write function to be implemented by subclasses.
    virtual bool writeRawBytes(const uint8_t *bytes, size_t len) = 0;

private:
    bool _fileLittleEndian;  // set by writeHeader(), used by writeUInt*()
};

} // namespace JB2
} // namespace OMR

#endif // defined(JBIRWRITER_INCL)
