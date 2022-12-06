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

#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Context.hpp"
#include "Extension.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"


namespace OMR {
namespace JitBuilder {


Context::Context(LOCATION, Compilation *comp, LiteralDictionary *useLitDict, SymbolDictionary *useSymDict, TypeDictionary *useTypeDict, uint32_t numEntryPoints, uint32_t numExitPoints, String name)
    : _id(comp->getContextID())
    , _comp(comp)
    , _name(name)
    , _parent(NULL)
    , _litDict((useLitDict != NULL) ? useLitDict : comp->litdict())
    , _symDict((useSymDict != NULL) ? useSymDict : comp->symdict())
    , _typeDict((useTypeDict != NULL) ? useTypeDict : comp->typedict())
    , _numEntryPoints(numEntryPoints)
    , _numExitPoints(numExitPoints) {

    initEntriesAndExits(PASSLOC, comp);
}

Context::Context(LOCATION, Context *parent, LiteralDictionary *useLitDict, SymbolDictionary *useSymDict, TypeDictionary *useTypeDict, uint32_t numEntryPoints, uint32_t numExitPoints, String name)
    : _id(parent->comp()->getContextID())
    , _comp(parent->comp())
    , _name(name)
    , _parent(parent)
    , _litDict(useLitDict)
    , _symDict(useSymDict)
    , _typeDict(useTypeDict)
    , _numEntryPoints(numEntryPoints)
    , _numExitPoints(numExitPoints) {

    parent->addChild(this);
    initEntriesAndExits(PASSLOC, parent->comp());
}

void
Context::initEntriesAndExits(LOCATION, Compilation *comp) {
    Extension *core = _comp->compiler()->lookupExtension<Extension>();

    _builderEntryPoints = new Builder *[_numEntryPoints];
    _nativeEntryPoints = new void *[_numEntryPoints];
    _debugEntryPoints = new void *[_numEntryPoints];
    for (unsigned e=0;e < _numEntryPoints;e++) {
        _builderEntryPoints[e] = core->EntryBuilder(PASSLOC, comp, this);
        _nativeEntryPoints[e] = 0;
        _debugEntryPoints[e] = 0;
    }

    _builderExitPoints = new Builder *[_numExitPoints];
    for (unsigned x=0;x < _numExitPoints;x++)
        _builderExitPoints[x] = core->ExitBuilder(PASSLOC, comp, this);
}

void
Context::addSymbol(Symbol *sym) {
    if (_symDict) {
        _symDict->registerSymbol(sym);
        return;
    }

    if (_parent) {
        _parent->addSymbol(sym);
        return;
    }

    assert(0); // there should be some symbol dictionary!
}

Symbol *
Context::lookupSymbol(String name, bool includeParents) {
    Symbol *sym = NULL;
    if (_symDict) {
        sym = _symDict->LookupSymbol(name);
    }

    if (sym == NULL) {
	if (includeParents && _parent != NULL)
            return this->_parent->lookupSymbol(name);
    }

    return sym;
}

} // namespace JitBuilder
} // namespace OMR
