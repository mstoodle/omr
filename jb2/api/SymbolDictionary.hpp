/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
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

#ifndef SYMBOLDICTIONARY_INCL
#define SYMBOLDICTIONARY_INCL

#include "common.hpp"
#include <map>
#include "ExtensibleIR.hpp"

namespace OMR {
namespace JitBuilder {

class Compiler;
class DebugDictionary;
class DynamicType;
class Extension;
class IR;
class IRCloner;
class OperationBuilder;
class TextLogger;
class Type;

class SymbolDictionary : public ExtensibleIR {
    JBALLOC_(SymbolDictionary)

    friend class Compiler;
    friend class DebugDictionary;
    friend class DynamicType;
    friend class Extension;
    friend class IR;
    friend class IRCloner;
    friend class OperationBuilder;

public:
    DYNAMIC_ALLOC_ONLY(SymbolDictionary, Compiler *compiler);
    DYNAMIC_ALLOC_ONLY(SymbolDictionary, Compiler *compiler, String name);
    DYNAMIC_ALLOC_ONLY(SymbolDictionary, Compiler *compiler, String name, SymbolDictionary *linkedTypes);

    SymbolListIterator symbolIterator() const { return _symbols.iterator(); }

    Symbol *LookupSymbol(SymbolID id);
    Symbol *LookupSymbol(String name);
    void RemoveSymbol(Symbol *symbol);

    SymbolDictionaryID id() const { return _id; }
    const String & name() const { return _name; }
    bool hasLinkedDictionary() const { return _linkedDictionary != NULL; }
    SymbolDictionary *linkedDictionary() const { return _linkedDictionary; }

    void registerSymbol(Symbol *symbol);

    void log(TextLogger &w) const;

    SymbolID numSymbols() const { return _nextSymbolID; }

protected:
    SymbolDictionary(Allocator *a, const SymbolDictionary *source, IRCloner *cloner);

    void internalRegisterSymbol(Symbol *symbol);
    virtual SymbolDictionary *clone(Allocator *mem, IRCloner *cloner) const;
    virtual void logContents(TextLogger &w) const {}

    SymbolDictionaryID _id;
    Compiler * _compiler;
    Allocator * _mem;
    String _name;
    SymbolList _symbols;
    SymbolList _ownedSymbols;
    std::map<const Type *,SymbolList *> _symbolsByType;
    std::map<String, Symbol *> _symbolsByName;
    SymbolID _nextSymbolID;
    SymbolDictionary * _linkedDictionary;

    SUBCLASS_KINDSERVICE_DECL(Extensible, SymbolDictionary);
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(SYMBOLDICTIONARY_INCL)
