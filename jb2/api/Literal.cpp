/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
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

#include <string.h>
#include "Compilation.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Literal, LiteralDictionary)

Literal::Literal(MEM_LOCATION(a), Compilation *comp, const Type *type, const LiteralBytes *v)
    : Allocatable(a)
    , _id(comp->litdict()->getLiteralID())
    , _creator(PASSLOC)
    , _litDict(comp->litdict())
    , _type(type) {

    // privatize the literal value
    size_t numBytes = (type->size() / 8) + ((type->size() & 7 > 0) ? 1 : 0);
    LiteralBytes *newBytes = reinterpret_cast<LiteralBytes *>(a->allocate(numBytes, NoAllocationCategory));
    memcpy(newBytes, v, numBytes);
    _pValue = newBytes;
}

Literal::Literal(MEM_LOCATION(a), LiteralDictionary *litDict, const Type *type, const LiteralBytes *v)
    : Allocatable(a)
    , _id(litDict->getLiteralID())
    , _creator(PASSLOC)
    , _litDict(litDict)
    , _type(type) {

    // privatize the literal value
    size_t numBytes = (type->size() / 8) + ((type->size() & 7 > 0) ? 1 : 0);
    LiteralBytes *newBytes = reinterpret_cast<LiteralBytes *>(a->allocate(numBytes, NoAllocationCategory));
    memcpy(newBytes, v, numBytes);
    _pValue = newBytes;
}

Literal::~Literal() {
    allocator()->deallocate(const_cast<void *>(reinterpret_cast<const void *>(this->_pValue)));
}

bool
Literal::operator==(Literal & other) {
    if (this->_type != other._type)
        return false;

    return this->_type->literalsAreEqual(this->_pValue, other._pValue);
}

void
Literal::log(TextLogger & lgr) const {
    lgr.indent() << this;
    #if 0
    lgr.indent() << this << " ";
    _type->logLiteral(lgr, this);
    lgr << " ]";
    #endif
}

const int64_t
Literal::getInteger() const {
    return _type->getInteger(this);
}

const double
Literal::getFloatingPoint() const {
    return _type->getFloatingPoint(this);
}

} // namespace JitBuilder
} // namespace OMR
