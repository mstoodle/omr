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

#include <cstring>
#include "Allocator.hpp"
#include "JBIRBufferWriter.hpp"

namespace OMR {
namespace JB2 {

JBIRBufferWriter::JBIRBufferWriter(Allocator *allocator, size_t initialCapacity)
    : JBIRWriter()
    , _allocator(allocator)
    , _buffer(nullptr)
    , _capacity(0)
    , _cursor(0) {
    if (_allocator != nullptr && initialCapacity > 0) {
        _buffer = static_cast<uint8_t *>(_allocator->allocate(initialCapacity, NoAllocationCategory));
        if (_buffer != nullptr) {
            _capacity = initialCapacity;
        }
    }
}

JBIRBufferWriter::~JBIRBufferWriter() {
    if (_allocator != nullptr && _buffer != nullptr) {
        _allocator->deallocate(_buffer);
        _buffer = nullptr;
    }
}

bool
JBIRBufferWriter::grow(size_t needed) {
    if (_allocator == nullptr)
        return false;

    size_t newCapacity = (_capacity == 0) ? kDefaultInitialCapacity : _capacity * 2;
    while (newCapacity < needed)
        newCapacity *= 2;

    uint8_t *newBuf = static_cast<uint8_t *>(
        _allocator->reallocate(_buffer, _capacity, newCapacity, NoAllocationCategory));
    if (newBuf == nullptr)
        return false;

    _buffer = newBuf;
    _capacity = newCapacity;
    return true;
}

bool
JBIRBufferWriter::writeRawBytes(const uint8_t *bytes, size_t len) {
    if (len == 0)
        return true;
    if (bytes == nullptr)
        return false;

    if (_cursor + len > _capacity) {
        if (!grow(_cursor + len))
            return false;
    }

    std::memcpy(_buffer + _cursor, bytes, len);
    _cursor += len;
    return true;
}

} // namespace JB2
} // namespace OMR
