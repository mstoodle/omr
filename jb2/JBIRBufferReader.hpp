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

// JBIRBufferReader.hpp — reads JBIR format data from a memory buffer.

#ifndef JBIRBUFFERREADER_INCL
#define JBIRBUFFERREADER_INCL

#include <cstddef>
#include <cstdint>
#include "JBIRReader.hpp"

namespace OMR {
namespace JB2 {

class JBIRBufferWriter;

class JBIRBufferReader : public JBIRReader {
public:
    // Construct a reader targeting a memory buffer of given length.
    JBIRBufferReader(const uint8_t *buffer, size_t len);
    JBIRBufferReader(const void *buffer, size_t len);
    explicit JBIRBufferReader(const JBIRBufferWriter &writer);

    // Number of bytes consumed so far.
    size_t bytesRead() const { return _cursor; }

    // Remaining bytes available to read.
    size_t remaining() const { return (_length > _cursor) ? (_length - _cursor) : 0; }

    // Total buffer length.
    size_t length() const { return _length; }

    // Pointer to current read position in the buffer.
    const uint8_t *current() const { return _buffer ? (_buffer + _cursor) : nullptr; }

protected:
    bool readRawBytes(uint8_t *buf, size_t len) override;

private:
    const uint8_t *_buffer;
    size_t         _length;
    size_t         _cursor;
};

using JBIRMemoryReader = JBIRBufferReader;

} // namespace JB2
} // namespace OMR

#endif // defined(JBIRBUFFERREADER_INCL)
