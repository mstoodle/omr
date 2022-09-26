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

#include <string>
#include "Compilation.hpp"
#include "Operation.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "TextWriter.hpp"
#include "Type.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {


LiteralDictionary::LiteralDictionary(Compilation *comp)
    : _id(comp->getLiteralDictionaryID())
    , _comp(comp)
    , _name("")
    , _nextLiteralID(NoLiteral+1)
    , _linkedDictionary(NULL) {
}

LiteralDictionary::LiteralDictionary(Compilation *comp, std::string name)
    : _id(comp->getLiteralDictionaryID())
    , _comp(comp)
    , _name(name)
    , _nextLiteralID(NoLiteral+1)
    , _linkedDictionary(NULL) {
}

// Only accessible to subclasses
LiteralDictionary::LiteralDictionary(Compilation *comp, std::string name, LiteralDictionary * linkedLiterals)
    : _id(comp->getLiteralDictionaryID())
    , _comp(comp)
    , _name(name)
    , _nextLiteralID(linkedLiterals->_nextLiteralID)
    , _linkedDictionary(linkedLiterals) {

    for (LiteralIterator it = linkedLiterals->LiteralsBegin(); it != linkedLiterals->LiteralsEnd(); it++) {
        Literal *literal = *it;
        addNewLiteral(literal);
    }
}

LiteralDictionary::~LiteralDictionary() {
    for (auto it = _ownedLiterals.begin(); it != _ownedLiterals.end(); it++) {
        Literal *literal = *it;
        delete literal;
    }
}

Literal *
LiteralDictionary::LookupLiteral(LiteralID id) {
    for (auto it = LiteralsBegin(); it != LiteralsEnd(); it++) {
        Literal *literal = *it;
        if (literal->id() == id)
            return literal;
    }

    return NULL;
}

void
LiteralDictionary::RemoveLiteral(Literal *literal) {
    // brutal performance; should really collect these and do in one pass
    for (auto it = _literals.begin(); it != _literals.end(); ) {
       if (*it == literal)
          it = _literals.erase(it);
       else
          ++it;
   }
}

void
LiteralDictionary::addNewLiteral(Literal *literal) {
    LiteralVector *typeList = NULL;
    const Type *type = literal->type();
    auto it = _literalsByType.find(type);
    if (it != _literalsByType.end()) {
        typeList = it->second;
    }
    else {
        typeList = new LiteralVector;
        _literalsByType.insert({type, typeList});
    }
    typeList->push_back(literal);
    _literals.push_back(literal);
}

Literal *
LiteralDictionary::registerLiteral(LOCATION, const Type *type, const LiteralBytes *value) {
    LiteralVector *typeList = NULL;
    auto it = _literalsByType.find(type);
    if (it != _literalsByType.end()) {
        typeList = it->second;
        for (auto it = typeList->begin(); it != typeList->end();it++) {
            Literal *other = *it;
            if (type->literalsAreEqual(value, other->value())) {
                return other;
            }
        }
    }
    else {
        typeList = new LiteralVector;
        _literalsByType.insert({type, typeList});
    }

    Literal *literal = new Literal(PASSLOC, _comp, type, value);
    typeList->push_back(literal);
    _literals.push_back(literal);
    _ownedLiterals.push_back(literal);
    return literal;
}

void
LiteralDictionary::write(TextWriter &w) {
    w.indent() << "[ LiteralDictionary " << this << " \"" << this->name() << "\"" << w.endl();
    w.indentIn();
    if (this->hasLinkedDictionary())
        w.indent() << "[ linkedDictionary " << this->linkedDictionary() << " ]" << w.endl();
    for (LiteralIterator literalIt = this->LiteralsBegin();literalIt != this->LiteralsEnd();literalIt++) {
        Literal *literal = *literalIt;
        literal->write(w);
        w << w.endl();
    }
    w.indentOut();
    w.indent() << "]" << w.endl();
}

} // namespace JitBuilder
} // namespace OMR

