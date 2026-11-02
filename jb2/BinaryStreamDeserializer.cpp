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
#include "BinaryStreamDeserializer.hpp"
#include "Extension.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(BinaryStreamDeserializer, Extension)
SUBCLASS_KINDSERVICE_IMPL(BinaryStreamDeserializer, "BinaryStreamDeserializer", BasicBinaryDeserializer, Extensible)

BinaryStreamDeserializer::BinaryStreamDeserializer(Allocator *mem, Extension *ext,
                                                   std::istream & is,
                                                   KINDTYPE(Extensible) kind)
    : BasicBinaryDeserializer(mem, ext, /*buf=*/nullptr, /*size=*/0, kind)
    , _is(is) {
}

BinaryStreamDeserializer::~BinaryStreamDeserializer() {
}

// Override consume to read directly from the stream into the staging buffer.
// For requests larger than kStagingSize the caller has a bug — staging is
// sized to hold any primitive (largest is double/uint64_t = 8 bytes).
const uint8_t *
BinaryStreamDeserializer::consume(size_t len) {
    assert(len <= kStagingSize && "BinaryStreamDeserializer: consume request exceeds staging buffer");
    _is.read(reinterpret_cast<char *>(_staging), static_cast<std::streamsize>(len));
    assert(_is.good() && "BinaryStreamDeserializer: read from istream failed");
    return _staging;
}

} // namespace JB2
} // namespace OMR
