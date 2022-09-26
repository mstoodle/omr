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
#include "TextWriter.hpp"
#include "Type.hpp"

namespace OMR {
namespace JitBuilder {

Literal::Literal(LOCATION, Compilation *comp, const Type *type, const LiteralBytes *v)
    : _id(comp->getLiteralID())
    , _creator(PASSLOC)
    , _comp(comp)
    , _type(type) {

    // privatize the literal value
    size_t numBytes = (type->size() / 8) + ((type->size() & 7 > 0) ? 1 : 0);
    LiteralBytes *newBytes = new LiteralBytes[numBytes];
    memcpy(newBytes, v, numBytes);
    _pValue = newBytes;
}

Literal::~Literal() {
    delete[] this->_pValue;
}

bool
Literal::operator==(Literal & other) {
    if (this->_type != other._type)
        return false;

    return this->_type->literalsAreEqual(this->_pValue, other._pValue);
}

void
Literal::write(TextWriter & w) const {
    w.indent() << this;
    #if 0
    w.indent() << this << " ";
    _type->printLiteral(w, this);
    w << " ]";
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
