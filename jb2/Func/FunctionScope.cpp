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

#include "Func/FunctionExtension.hpp"
#include "Func/FunctionScope.hpp"


namespace OMR {
namespace JB2 {
namespace Func {

INIT_JBALLOC(FunctionScope)
SUBCLASS_KINDSERVICE_IMPL(FunctionScope, "FunctionScope", Scope, Extensible)

FunctionScope::FunctionScope(Allocator *a, Extension *ext, IR *ir, String name)
    : Scope(a, ext, KIND(Extensible), ir, name) {

    Builder *entryBuilder = ir->unit()->EntryBuilder(LOC, ir, this);
    BuilderEntry *entry = new (a) BuilderEntry(a, 0, entryBuilder);
    addEntryPoint(entry);
}

FunctionScope::FunctionScope(Allocator *a, Extension *ext, Scope *parent, String name)
    : Scope(a, ext, KIND(Extensible), parent, name) {

}

FunctionScope::FunctionScope(Allocator *a, const FunctionScope *source, IRCloner *cloner)
    : Scope(a, source, cloner) {

}

FunctionScope::~FunctionScope() {

}

Scope *
FunctionScope::clone(Allocator *mem, IRCloner *cloner) const {
    return new (mem) FunctionScope(mem, this, cloner);
}

} // namespace Func
} // namespace JB2
} // namespace OMR
