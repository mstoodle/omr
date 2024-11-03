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
#include "IRCloner.hpp"
#include "Operation.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(LiteralDictionary, Dictionaries)
SUBCLASS_KINDSERVICE_IMPL(LiteralDictionary, "LiteralDictionary", ExtensibleIR, Extensible)

LiteralDictionary::LiteralDictionary(Allocator *a, Compiler *compiler)
    : Extensible(a, compiler->coreExt(), getExtensibleClassKind())
    , _id(compiler->getLiteralDictionaryID())
    , _compiler(compiler)
    , _mem(a)
    , _name("")
    , _literals(NULL, _mem)
    , _ownedLiterals(NULL, _mem)
    , _nextLiteralID(NoLiteral+1)
    , _linkedDictionary(NULL) {
}

LiteralDictionary::LiteralDictionary(Allocator *a, Compiler *compiler, String name)
    : Extensible(a, compiler->coreExt(), getExtensibleClassKind())
    , _id(compiler->getLiteralDictionaryID())
    , _compiler(compiler)
    , _mem(a)
    , _name(name)
    , _literals(NULL, _mem)
    , _ownedLiterals(NULL, _mem)
    , _nextLiteralID(NoLiteral+1)
    , _linkedDictionary(NULL) {
}

LiteralDictionary::LiteralDictionary(Allocator *a, Compiler *compiler, String name, LiteralDictionary * linkedLiterals)
    : Extensible(a, compiler->coreExt(), getExtensibleClassKind())
    , _id(compiler->getLiteralDictionaryID())
    , _compiler(compiler)
    , _mem(a)
    , _name(name)
    , _literals(NULL, _mem)
    , _ownedLiterals(NULL, _mem)
    , _nextLiteralID(linkedLiterals->_nextLiteralID)
    , _linkedDictionary(linkedLiterals) {

    for (auto it = linkedLiterals->literalIterator(); it.hasItem(); it++) {
        Literal *literal = it.item();
        addNewLiteral(literal);
    }
}

// only used by clone
LiteralDictionary::LiteralDictionary(Allocator *a, const LiteralDictionary *source, IRCloner *cloner)
    : Extensible(a, source->ext(), source->kind())
    , _id(source->_id)
    , _compiler(source->_compiler)
    , _mem(a)
    , _name(source->_name)
    , _literals(NULL, _mem)
    , _ownedLiterals(NULL, _mem)
    , _nextLiteralID(source->_nextLiteralID)
    , _linkedDictionary(cloner->clonedLiteralDictionary(source->_linkedDictionary)) {

    for (auto it = source->literalIterator(); it.hasItem(); it++) {
        Literal *literal = it.item();
        addNewLiteral(cloner->clonedLiteral(literal));
    }
}
    
LiteralDictionary::~LiteralDictionary() {
    for (auto it = _ownedLiterals.iterator(); it.hasItem(); it++) {
        Literal *literal = it.item();
        delete literal;
    }
    _ownedLiterals.erase();

    for (auto it = _literalsByType.begin(); it != _literalsByType.end(); it++) {
        LiteralList *tl = it->second;
        delete tl;
    }
}

LiteralDictionary *
LiteralDictionary::clone(Allocator *mem, IRCloner *cloner) const {
    return new (mem) LiteralDictionary(mem, this, cloner);
}

Literal *
LiteralDictionary::LookupLiteral(LiteralID id) {
    for (auto it = literalIterator(); it.hasItem(); it++) {
        Literal *literal = it.item();
        if (literal->id() == id)
            return literal;
    }

    return NULL;
}

void
LiteralDictionary::RemoveLiteral(Literal *literal) {
    auto it = _literals.find(literal);
    if (it.hasItem())
        _literals.remove(it);
}

void
LiteralDictionary::addNewLiteral(Literal *literal) {
    LiteralList *litList = NULL;
    const Type *type = literal->type();
    auto it = _literalsByType.find(type);
    if (it != _literalsByType.end()) {
        litList = it->second;
    }
    else {
        litList = new (_mem) LiteralList(_mem);
        _literalsByType.insert({type, litList});
    }
    litList->push_back(literal);
    _literals.push_back(literal);
}

Literal *
LiteralDictionary::registerLiteral(LOCATION, const Type *type, const LiteralBytes *value) {
    LiteralList *typeList = NULL;
    auto it = _literalsByType.find(type);
    if (it != _literalsByType.end()) {
        typeList = it->second;
        for (auto it = typeList->iterator(); it.hasItem();it++) {
            Literal *other = it.item();
            if (type->literalsAreEqual(value, other->value())) {
                return other;
            }
        }
    }
    else {
        typeList = new (_mem) LiteralList(_mem);
        _literalsByType.insert({type, typeList});
    }

    Literal *literal = new (_mem) Literal(MEM_PASSLOC(_mem), this, type, value);
    typeList->push_back(literal);
    _literals.push_back(literal);
    _ownedLiterals.push_back(literal);

    return literal;
}

void
LiteralDictionary::log(TextLogger &lgr) {
    lgr.irSectionBegin("litdict", "L", id(), kind(), name());
    lgr.irFlagOrNull<LiteralDictionary>("linkedDictionary", this->linkedDictionary());
    logContents(lgr);
    for (auto it = this->literalIterator();it.hasItem();it++) {
        Literal *literal = it.item();
        literal->log(lgr);
    }
    lgr.irSectionEnd();
}

void
LiteralDictionary::logContents(TextLogger &lgr) {
}

} // namespace JitBuilder
} // namespace OMR

