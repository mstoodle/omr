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

// JBIR file format — header section specification
//
// The header is the first 12 bytes of every JBIR file:
//
//   Offset  Size  Type      Field
//   ------  ----  --------  -----
//   0       4     uint8[4]  magic = { 'J','B','I','R' }
//   4       1     uint8     flags0
//                             bit 0: 1 = file written little-endian
//                                    0 = file written big-endian
//                             bits 1-7: reserved, must be zero
//   5       1     uint8     flags1  — reserved, must be zero
//   6       2     uint16    version_major  (endianness per flags0 bit 0)
//   8       2     uint16    version_minor
//   10      2     uint16    version_patch
//
// The magic bytes and both flags bytes are always read as raw bytes.
// Every multi-byte integer in the file (including in the header from offset 6
// onward) uses the byte order recorded in flags0 bit 0.
// The current format version is 2.0.0.

#ifndef JBIRFILEHEADER_INCL
#define JBIRFILEHEADER_INCL

#include <cstdint>

namespace OMR {
namespace JB2 {

// Magic bytes: 'J','B','I','R'
static const uint8_t JBIR_MAGIC[4] = { 'J', 'B', 'I', 'R' };

// flags0 bit masks
static const uint8_t JBIR_FLAG0_LITTLE_ENDIAN = 0x01u;

// Current format version
static const uint16_t JBIR_VERSION_MAJOR = 2;
static const uint16_t JBIR_VERSION_MINOR = 0;
static const uint16_t JBIR_VERSION_PATCH = 0;

// Total size of the serialized header in bytes
static const uint32_t JBIR_HEADER_SIZE = 12;

//
// JBIRFileHeader holds the in-memory representation of the file header.
// It is a plain value type with no allocation dependency.
//
// versionMajor/Minor/Patch are always in host byte order here;
// the writer/reader handle conversion to/from the on-disk byte order
// as recorded in flags0 bit 0.
//
struct JBIRFileHeader {
    uint8_t  magic[4];
    uint8_t  flags0;
    uint8_t  flags1;
    uint16_t versionMajor;
    uint16_t versionMinor;
    uint16_t versionPatch;

    bool isLittleEndian() const { return (flags0 & JBIR_FLAG0_LITTLE_ENDIAN) != 0; }

    // Construct a header stamped with the current format version and the
    // endianness of the platform this code is running on.
    static JBIRFileHeader current();
};

} // namespace JB2
} // namespace OMR

#endif // defined(JBIRFILEHEADER_INCL)
