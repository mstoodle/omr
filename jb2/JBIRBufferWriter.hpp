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

// JBIRBufferWriter.hpp — writes JBIR format data to a dynamically growing memory buffer allocated via an Allocator.

#ifndef JBIRBUFFERWRITER_INCL
#define JBIRBUFFERWRITER_INCL

#include <cstddef>
#include <cstdint>
#include "JBIRWriter.hpp"

namespace OMR {
namespace JB2 {

class Allocator;

class JBIRBufferWriter : public JBIRWriter {
public:
    static constexpr size_t kDefaultInitialCapacity = 256;

    // Construct a buffer writer using an Allocator to allocate and grow its internal buffer.
    explicit JBIRBufferWriter(Allocator *allocator, size_t initialCapacity = kDefaultInitialCapacity);
    ~JBIRBufferWriter() override;

    // Direct access to the written buffer bytes.
    const uint8_t *buffer() const { return _buffer; }
    uint8_t *buffer() { return _buffer; }

    // Number of bytes written so far.
    size_t bytesWritten() const { return _cursor; }
    size_t size() const { return _cursor; }

    // Current buffer capacity.
    size_t capacity() const { return _capacity; }

    // Allocator used.
    Allocator *allocator() const { return _allocator; }

protected:
    bool writeRawBytes(const uint8_t *bytes, size_t len) override;

private:
    bool grow(size_t needed);

    Allocator *_allocator;
    uint8_t   *_buffer;
    size_t     _capacity;
    size_t     _cursor;
};

using JBIRMemoryWriter = JBIRBufferWriter;

} // namespace JB2
} // namespace OMR

#endif // defined(JBIRBUFFERWRITER_INCL)
