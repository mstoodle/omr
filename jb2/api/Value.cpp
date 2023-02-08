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

#include "AllocationCategoryClasses.hpp"
#include "Builder.hpp"
#include "Compilation.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Value, IL)

Value *
Value::create(const Builder * parent, const Type * type) {
    Allocator *mem = parent->comp()->mem();
    Value *value = new (mem) Value(mem, parent, type);
    parent->comp()->rememberNewValue(value);
    return value;
}

Value::Value(Allocator *a, const Builder * parent, const Type * type)
    : Allocatable(a)
    , _id(parent->comp()->getValueID())
    , _parent(parent)
    , _type(type)
    , _definitions(NULL, a) {

}

Value::~Value() {
}

} // namespace JitBuilder
} // namespace OMR
