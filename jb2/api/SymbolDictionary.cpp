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
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(SymbolDictionary, Dictionaries)
SUBCLASS_KINDSERVICE_IMPL(SymbolDictionary, "SymbolDictionary", Extensible, Extensible)

SymbolDictionary::SymbolDictionary(Allocator *a, Compiler *compiler)
    : Extensible(a, compiler->coreExt(), getExtensibleClassKind())
    , _id(compiler->getSymbolDictionaryID())
    , _compiler(compiler)
    , _mem(a)
    , _name("")
    , _symbols(NULL, _mem)
    , _ownedSymbols(NULL, _mem)
    , _nextSymbolID(NoSymbol+1)
    , _linkedDictionary(NULL) {
}

SymbolDictionary::SymbolDictionary(Allocator *a, Compiler *compiler, String name)
    : Extensible(a, compiler->coreExt(), getExtensibleClassKind())
    , _id(compiler->getSymbolDictionaryID())
    , _compiler(compiler)
    , _mem(a)
    , _name(name)
    , _symbols(NULL, _mem)
    , _ownedSymbols(NULL, _mem)
    , _nextSymbolID(NoSymbol+1)
    , _linkedDictionary(NULL) {
}

SymbolDictionary::SymbolDictionary(Allocator *a, Compiler *compiler, String name, SymbolDictionary * linkedDictionary)
    : Extensible(a, compiler->coreExt(), getExtensibleClassKind())
    , _id(compiler->getSymbolDictionaryID())
    , _compiler(compiler)
    , _mem(a)
    , _name(name)
    , _symbols(NULL, _mem)
    , _ownedSymbols(NULL, _mem)
    , _nextSymbolID(linkedDictionary->_nextSymbolID)
    , _linkedDictionary(linkedDictionary) {

    for (auto it = linkedDictionary->symbolIterator(); it.hasItem(); it++) {
         Symbol *sym = it.item();
         internalRegisterSymbol(sym);
    }
    _nextSymbolID = linkedDictionary->_nextSymbolID;
}

// only used by clone()
SymbolDictionary::SymbolDictionary(Allocator *a, const SymbolDictionary * source, IRCloner *cloner)
    : Extensible(a, source->ext(), source->kind())
    , _id(source->_id)
    , _compiler(source->_compiler)
    , _mem(a)
    , _name(source->_name)
    , _symbols(NULL, a)
    , _ownedSymbols(NULL, a)
    , _nextSymbolID(source->_nextSymbolID)
    , _linkedDictionary(cloner->clonedSymbolDictionary(source->_linkedDictionary)) {

    for (auto it = source->symbolIterator(); it.hasItem(); it++) {
         Symbol *sym = it.item();
         internalRegisterSymbol(sym->cloneSymbol(a, cloner));
    }
}

SymbolDictionary::~SymbolDictionary() {
    for (auto it = _ownedSymbols.iterator(); it.hasItem(); it++) {
        Symbol *sym = it.item();
        delete sym;
    }
    _ownedSymbols.erase();

    for (auto it = _symbolsByType.begin(); it != _symbolsByType.end(); it++) {
        SymbolList *tl = it->second;
        delete tl;
    }
}

SymbolDictionary *
SymbolDictionary::cloneDictionary(Allocator *a, IRCloner *cloner) const {
    return new (a) SymbolDictionary(a, this, cloner);
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
    SymbolList *symList = NULL;
    const Type *type = symbol->type();
    auto it = _symbolsByType.find(type);
    if (it != _symbolsByType.end()) {
        symList = it->second;
    }
    else {
        symList = new (_mem) SymbolList(_mem);
        _symbolsByType.insert({type, symList});
    }
    symList->push_back(symbol);
    _symbolsByName.insert({symbol->name(), symbol});
    _symbols.push_back(symbol);
}

// for symbols that should be owned by this SymbolDictionary
void
SymbolDictionary::registerSymbol(Symbol *symbol) {
    symbol->assignID(_nextSymbolID++);
    internalRegisterSymbol(symbol);
    _ownedSymbols.push_back(symbol);
}

void
SymbolDictionary::log(TextLogger &lgr) const {
    lgr.irSectionBegin("symdict", "S", id(), kind(), name());
    lgr.irFlagOrNull<SymbolDictionary>("linkedDictionary", this->linkedDictionary());
    logContents(lgr);
    for (auto it = this->symbolIterator();it.hasItem();it++) {
        Symbol *symbol = it.item();
        lgr.indent();
        symbol->log(lgr);
    }
    lgr.irSectionEnd();
}

} // namespace JitBuilder
} // namespace OMR
