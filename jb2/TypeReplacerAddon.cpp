/*******************************************************************************
 * Copyright (c) 2026 IBM Corp. and others
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

#include "TypeReplacer.hpp"
#include "TypeReplacerAddon.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(TypeReplacerAddon, TypeReplacement)
SUBCLASS_KINDSERVICE_IMPL(TypeReplacerAddon, "TypeReplacerAddon", Addon, Extensible)

TypeReplacerAddon::TypeReplacerAddon(Allocator *a, Extension *ext, TypeReplacer *repl, KINDTYPE(Extensible) kind)
        : Addon(a, ext, repl, kind) {

    }

TypeReplacerAddon::~TypeReplacerAddon() {

}

void
TypeReplacerAddon::collectTypes(IR *ir) const {
}

bool
TypeReplacerAddon::hasExpander(Operation *op) const {
    return false;
}

bool
TypeReplacerAddon::expand(Builder *b, Operation *op, OperationReplacer *r) const {
    return false;
}

} // namespace JB2
} // namespace OMR
