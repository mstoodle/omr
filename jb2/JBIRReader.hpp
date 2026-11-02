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

// JBIRReader.hpp — abstract base class for JBIR deserialization readers.
//
// Inherits JBIR (from JBIRWriter.hpp) for the shared endianness helpers.
// Subclasses provide readRawBytes().

#ifndef JBIRREADER_INCL
#define JBIRREADER_INCL

#include <cstddef>
#include <cstdint>
#include "JBIRWriter.hpp"   // for JBIR base class

namespace OMR {
namespace JB2 {

class JBIRReader : public JBIR {
public:
    virtual ~JBIRReader() = default;

    // Read and validate the JBIR file header into h.
    // Returns true on success.  On failure h is left in an unspecified state.
    // Validation checks:
    //   - magic bytes match { 'J','B','I','R' }
    //   - flags1 is zero (reserved bits must be zero)
    //   - flags0 reserved bits (1-7) are zero
    bool readHeader(JBIRFileHeader &h);

    // Read integers in the byte order established by the last readHeader().
    // Must be called after a successful readHeader().
    bool readUInt16(uint16_t &v);
    bool readUInt32(uint32_t &v);
    bool readUInt64(uint64_t &v);

protected:
    JBIRReader();

    // Pure virtual read function to be implemented by subclasses.
    virtual bool readRawBytes(uint8_t *buf, size_t len) = 0;

private:
    bool _fileLittleEndian;  // set by readHeader(), used by readUInt*()
};

} // namespace JB2
} // namespace OMR

#endif // defined(JBIRREADER_INCL)
