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

#ifndef CONTEXT_INCL
#define CONTEXT_INCL

#include <assert.h>
#include <list>
#include <string>
#include "IDs.hpp"

namespace OMR {
namespace JitBuilder {

class Compilation;
class LiteralDictionary;
class Symbol;
class SymbolDictionary;
class TypeDictionary;

class Context {

public:
    Context(LOCATION, Compilation *comp, LiteralDictionary *useLitDict=NULL, SymbolDictionary *useSymDict=NULL, TypeDictionary *useTypeDict=NULL, uint32_t numEntryPoints=1, uint32_t numExitPoints=1, std::string name="");
    Context(LOCATION, Context *parent, LiteralDictionary *useLitDict=NULL, SymbolDictionary *useSymDict=NULL, TypeDictionary *useTypeDict=NULL, uint32_t numEntryPoints=1, uint32_t numExitPoints=1, std::string name="");

    ContextID id() const { return _id; }
    std::string name() const { return _name; }

    LiteralDictionary *litDict() const { return _litDict; }
    SymbolDictionary *symDict() const { return _symDict; }
    TypeDictionary *typeDict() const { return _typeDict; }

    uint32_t numEntryPoints() const { return _numEntryPoints; }
    uint32_t numExitPoints() const { return _numExitPoints; }

    void * nativeEntryPoint(unsigned e=0) const { assert(e < _numEntryPoints); return _nativeEntryPoints[e]; }
    void setNativeEntryPoint(void *entry, unsigned e=0) {
        assert(e < _numEntryPoints);
        _nativeEntryPoints[e] = entry;
    }

    void * debugEntryPoint(unsigned e=0) const { assert(e < _numEntryPoints); return _debugEntryPoints[e]; }
    void setDebugEntryPoint(void *entry, unsigned e=0) {
        assert(e < _numEntryPoints);
        _debugEntryPoints[e] = entry;
    }

    Builder *builderEntryPoint(unsigned e=0) const {
        assert(e < _numEntryPoints);
        return _builderEntryPoints[e];
    }

    Builder *builderExitPoint(unsigned x=0) const {
        assert(x < _numExitPoints);
        return _builderExitPoints[x];
    }

    Symbol *lookupSymbol(std::string name, bool includeParents=true);

protected:
    void initEntriesAndExits(LOCATION, Compilation *comp);
    void addSymbol(Symbol *sym);
    void addChild(Context *child) { _children.push_back(child); }
    Compilation *comp() const { return _comp; }

    uint64_t _id;
    Compilation *_comp;
    std::string _name;

    Context * _parent;
    std::list<Context *> _children;

    LiteralDictionary *_litDict;
    SymbolDictionary *_symDict;
    TypeDictionary *_typeDict;

    uint32_t _numEntryPoints;
    void **_nativeEntryPoints;
    void **_debugEntryPoints;
    Builder **_builderEntryPoints;

    uint32_t _numExitPoints;
    Builder **_builderExitPoints;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(CONTEXT_INCL)

