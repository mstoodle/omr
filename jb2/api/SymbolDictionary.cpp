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

#include "Compilation.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextWriter.hpp"
#include "Type.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

SymbolDictionary::SymbolDictionary(Compilation *comp)
    : _id(comp->getSymbolDictionaryID())
    , _comp(comp)
    , _name("")
    , _nextSymbolID(NoSymbol+1)
    , _linkedDictionary(NULL) {

}

SymbolDictionary::SymbolDictionary(Compilation *comp, String name)
    : _id(comp->getSymbolDictionaryID())
    , _comp(comp)
    , _name(name)
    , _nextSymbolID(NoSymbol+1)
    , _linkedDictionary(NULL) {

}

// Only accessible to subclasses
SymbolDictionary::SymbolDictionary(Compilation *comp, String name, SymbolDictionary * linkedDictionary)
    : _id(comp->getSymbolDictionaryID())
    , _comp(comp)
    , _name(name)
    , _nextSymbolID(linkedDictionary->_nextSymbolID)
    , _linkedDictionary(linkedDictionary) {

    for (auto it = linkedDictionary->symbolIterator(); it.hasItem(); it++) {
         Symbol *sym = it.item();
         internalRegisterSymbol(sym);
    }
    _nextSymbolID = linkedDictionary->_nextSymbolID;
}

SymbolDictionary::~SymbolDictionary() {
    for (auto it = _ownedSymbols.iterator(); it.hasItem(); it++) {
        Symbol *sym = it.item();
        delete sym;
    }
}

Symbol *
SymbolDictionary::LookupSymbol(uint64_t id) {
    for (auto it = symbolIterator(); it.hasItem(); it++) {
        Symbol *sym = it.item();
        if (sym->id() == id)
            return sym;
    }

    return NULL;
}

Symbol *
SymbolDictionary::LookupSymbol(String name) {
    auto it = _symbolsByName.find(name);
    if (it != _symbolsByName.end())
        return it->second;

    return NULL;
}

void
SymbolDictionary::RemoveSymbol(Symbol *sym) {
    auto it = _symbols.find(sym);
    if (it.hasItem())
        _symbols.remove(it);
}

void
SymbolDictionary::internalRegisterSymbol(Symbol *symbol) {
    SymbolList *typeList = NULL;
    const Type *type = symbol->type();
    auto it = _symbolsByType.find(type);
    if (it != _symbolsByType.end()) {
        typeList = it->second;
    }
    else {
        typeList = new SymbolList;
        _symbolsByType.insert({type, typeList});
    }
    typeList->push_back(symbol);
    _symbolsByName.insert({symbol->name(), symbol});
    _symbols.push_back(symbol);
}

void
SymbolDictionary::registerSymbol(Symbol *symbol) {
    symbol->assignID(_nextSymbolID++);
    internalRegisterSymbol(symbol);
    _ownedSymbols.push_back(symbol);
}

void
SymbolDictionary::write(TextWriter &w) {
    w.indent() << "[ SymbolDictionary " << this << " \"" << this->name() << "\"" << w.endl();
    w.indentIn();
    if (this->hasLinkedDictionary())
        w.indent() << "[ linkedDictionary " << this->linkedDictionary() << " ]" << w.endl();

    for (auto it = this->symbolIterator();it.hasItem();it++) {
        Symbol *symbol = it.item();
        w.indent();
        symbol->write(w);
    }

    w.indentOut();
    w.indent() << "]" << w.endl();
}

} // namespace JitBuilder
} // namespace OMR
