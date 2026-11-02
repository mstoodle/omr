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

#ifndef BINARYSTREAMDESERIALIZER_INCL
#define BINARYSTREAMDESERIALIZER_INCL

#include <istream>
#include "BasicBinaryDeserializer.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// BinaryStreamDeserializer extends BasicBinaryDeserializer to read directly
// from an std::istream rather than from a pre-loaded memory buffer.
//
// It overrides the protected consume() path so that every byte read by the
// BasicBinaryDeserializer decoding methods is pulled from the stream.  A
// small staging buffer is used internally to satisfy consume() calls.
//
// Use this when the source is a file or socket and loading an entire
// serialized IR into memory before decoding is undesirable.
//

class BinaryStreamDeserializer : public BasicBinaryDeserializer {
    JBALLOC_(BinaryStreamDeserializer)

public:
    DYNAMIC_ALLOC_ONLY(BinaryStreamDeserializer, Extension *ext, std::istream & is,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

protected:
    // Override consume to read directly from the stream into the staging buffer,
    // then return a pointer to those bytes.
    virtual const uint8_t * consume(size_t len);

    std::istream & _is;

    // Small staging buffer for consume(); avoids a per-call heap allocation.
    static const size_t kStagingSize = 64;
    uint8_t _staging[kStagingSize];

    SUBCLASS_KINDSERVICE_DECL(Extensible, BinaryStreamDeserializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(BINARYSTREAMDESERIALIZER_INCL)
