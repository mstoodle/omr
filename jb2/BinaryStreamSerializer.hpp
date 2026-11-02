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

#ifndef BINARYSTREAMSERIALIZER_INCL
#define BINARYSTREAMSERIALIZER_INCL

#include <ostream>
#include "BasicBinarySerializer.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// BinaryStreamSerializer extends BasicBinarySerializer to write directly to
// an std::ostream rather than accumulating bytes in memory.
//
// It overrides the protected append() path so that every byte written by the
// BasicBinarySerializer encoding methods goes straight to the stream.  No
// in-memory buffer is maintained; data() / size() are not meaningful.
//
// Use this when the destination is a file or socket and buffering an entire
// serialized IR in memory is undesirable.
//

class BinaryStreamSerializer : public BasicBinarySerializer {
    JBALLOC_(BinaryStreamSerializer)

public:
    DYNAMIC_ALLOC_ONLY(BinaryStreamSerializer, Extension *ext, std::ostream & os,
                       KINDTYPE(Extensible) kind=KIND(Extensible));

protected:
    // Override append to write directly to the stream instead of the buffer.
    virtual void append(const uint8_t *bytes, size_t len);

    std::ostream & _os;

    SUBCLASS_KINDSERVICE_DECL(Extensible, BinaryStreamSerializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(BINARYSTREAMSERIALIZER_INCL)
