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

#include <cassert>
#include <cstring>
#include "Allocator.hpp"
#include "BinarySerializer.hpp"
#include "Extension.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(BinarySerializer, Extension)
SUBCLASS_KINDSERVICE_IMPL(BinarySerializer, "BinarySerializer", IRSerializer, Extensible)

BinarySerializer::BinarySerializer(Allocator *mem, Extension *ext, KINDTYPE(Extensible) kind)
    : IRSerializer(mem, ext, kind)
    , _buf(nullptr)
    , _size(0)
    , _capacity(0) {
}

BinarySerializer::~BinarySerializer() {
}


void
BinarySerializer::append(const uint8_t *bytes, size_t len) {
    if (_size + len > _capacity)
        grow(_size + len);
    memcpy(_buf + _size, bytes, len);
    _size += len;
}

void
BinarySerializer::grow(size_t needed) {
    size_t newCapacity = (_capacity == 0) ? kInitialCapacity : _capacity * 2;
    while (newCapacity < needed)
        newCapacity *= 2;

    uint8_t *newBuf = static_cast<uint8_t *>(
        allocator()->allocate(newCapacity, NoAllocationCategory));
    if (_buf != nullptr) {
        memcpy(newBuf, _buf, _size);
        allocator()->deallocate(_buf);
    }
    _buf = newBuf;
    _capacity = newCapacity;
}

} // namespace JB2
} // namespace OMR
