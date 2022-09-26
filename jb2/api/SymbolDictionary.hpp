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

#include <stdint.h>
#include <map>
#include <vector>
#include "IDs.hpp"
#include "Iterator.hpp"
#include "typedefs.hpp"

namespace OMR {
namespace JitBuilder {

class Compilation;
class Compiler;
class DebugDictionary;
class DynamicType;
class Extension;
class OperationBuilder;
class Type;

class SymbolDictionary {
    friend class DebugDictionary;
    friend class DynamicType;
    friend class Extension;
    friend class OperationBuilder;

public:
    SymbolDictionary(Compilation *comp);
    SymbolDictionary(Compilation *comp, std::string name);
    SymbolDictionary(Compilation *comp, std::string name, SymbolDictionary *linkedTypes);
    virtual ~SymbolDictionary();

    SymbolIterator SymbolsBegin() const { return SymbolIterator(_symbols); }
    SymbolIterator SymbolsEnd() const { return SymbolIterator(); }

    Symbol *LookupSymbol(SymbolID id);
    void RemoveSymbol(Symbol *symbol);

    SymbolDictionaryID id() const { return _id; }
    std::string name() const { return _name; }
    bool hasLinkedDictionary() const { return _linkedDictionary != NULL; }
    SymbolDictionary *linkedDictionary() { return _linkedDictionary; }

    void registerSymbol(Symbol *symbol);

    void write(TextWriter &w);

protected:
    void internalRegisterSymbol(Symbol *symbol);

    SymbolDictionaryID _id;
    Compilation * _comp;
    std::string _name;
    SymbolVector _symbols;
    SymbolVector _ownedSymbols;
    std::map<const Type *,SymbolVector *> _symbolsByType;
    SymbolID _nextSymbolID;
    SymbolDictionary * _linkedDictionary;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(SYMBOLDICTIONARY_INCL)

