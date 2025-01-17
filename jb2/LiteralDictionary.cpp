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
#include "Compiler.hpp"
#include "CoreExtension.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"
#include "Operation.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(LiteralDictionary, Dictionaries)
SUBCLASS_KINDSERVICE_IMPL(LiteralDictionary, "LiteralDictionary", ExtensibleIR, Extensible)

LiteralDictionary::LiteralDictionary(Allocator *a, IR *ir, String name)
    : DictBaseType(a, ir->ext(), ir, name, CLASSKIND(LiteralDictionary, Extensible)) {

}

// only used by clone
LiteralDictionary::LiteralDictionary(Allocator *a, const LiteralDictionary *source, IRCloner *cloner)
    : DictBaseType(a, source, cloner) {

}
 
LiteralDictionary::~LiteralDictionary() {
}

LiteralDictionary *
LiteralDictionary::cloneDictionary(Allocator *mem, IRCloner *cloner) const {
    return new (mem) LiteralDictionary(mem, this, cloner);
}

Literal *
LiteralDictionary::registerLiteral(LOCATION, const Type *type, const LiteralBytes *value) {
    IR *ir = type->ir();
    Allocator *mem = ir->mem();
    TypeID typeID = type->id();
    if (_entriesByType.exists(typeID)) {
        LiteralList *literalList = _entriesByType[typeID];
        if (literalList != NULL) {
            for (auto it = literalList->iterator(); it.hasItem();it++) {
                Literal *other = it.item();
                if (type->literalsAreEqual(value, other->value())) {
                    // found existing literal, so return that one
                    return other;
                }
            }
        }
    }
    Literal *literal = new (mem) Literal(MEM_PASSLOC(mem), ir, type, value);
    addNewEntry(literal);
    return literal;
}

} // namespace JB2
} // namespace OMR
