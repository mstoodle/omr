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

#include "Compiler.hpp"
#include "CoreExtension.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(TypeDictionary, Dictionaries)
SUBCLASS_KINDSERVICE_IMPL(TypeDictionary, "TypeDictionary", ExtensibleIR, Extensible)

TypeDictionary::TypeDictionary(Allocator *a, IR *ir, String name)
    : DictBaseType(a, ir->ext(), ir, name, CLASSKIND(TypeDictionary, Extensible)) {

}

TypeDictionary::TypeDictionary(Allocator *a, const TypeDictionary *source, IRCloner *cloner)
    : DictBaseType(a, source, cloner) {
}

TypeDictionary::~TypeDictionary() {
}

TypeDictionary *
TypeDictionary::cloneDictionary(Allocator *mem, IRCloner *cloner) const {
    return new (mem) TypeDictionary(mem, this, cloner);
}

} // namespace JB2
} // namespace OMR
