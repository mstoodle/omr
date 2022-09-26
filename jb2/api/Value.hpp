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

#ifndef VALUE_INCL
#define VALUE_INCL

#include <stdint.h>
#include <list>
#include "IDs.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Extension;
class OperationCloner;
class Type;

class Value {
    friend class Builder;
    friend class Extension;
    friend class Operation;
    friend class OperationCloner;

public:
    ValueID id() const { return _id; }
    const Builder *parent() const { return _parent; }
    const Type * type() const { return _type; }

    virtual size_t size() const { return sizeof(Value); }

protected:
    static Value * create(const Builder * parent, const Type * type);
    Value(const Builder * parent, const Type * type);
    void addDefinition(const Operation *op) { _definitions.push_back(op); }

    ValueID   _id;
    const Builder * _parent;
    const Type * _type;
    std::list<const Operation *> _definitions;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(VALUE_INCL)

