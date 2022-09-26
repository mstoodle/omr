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

#ifndef LITERAL_INCL
#define LITERAL_INCL

#include <map>
#include <stdint.h>
#include <vector>
#include "CreateLoc.hpp"
#include "IDs.hpp"
#include "typedefs.hpp"

namespace OMR {
namespace JitBuilder {

class Compilation;
class TextWriter;
class Type;

class Literal {
    friend class Compilation;

public:
    Literal(LOCATION, Compilation *comp, const Type *t, const LiteralBytes *v);
    ~Literal();

    LiteralID id() const { return _id; }
    const Type *type() const { return _type; }
    template<typename T>
    const T value() const { return *reinterpret_cast<T *>(_pValue); }
    const LiteralBytes *value() const { return _pValue; }
    void write(TextWriter & w) const;
    bool operator==(Literal & other);

    const int64_t getInteger() const;
    const double getFloatingPoint() const;

protected:
    LiteralID _id;
    CreateLocation _creator;
    Compilation *_comp;
    const Type *_type;
    const LiteralBytes *_pValue;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(LITERAL_INCL)

