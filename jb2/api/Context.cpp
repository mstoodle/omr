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

#include "Builder.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Context.hpp"
#include "Extension.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"


namespace OMR {
namespace JitBuilder {

INIT_JBALLOC(Context)
BASECLASS_KINDSERVICE_IMPL(Context)

Context::Context(LOCATION, ContextKind kind, Extension *ext, Compilation *comp, String name) //LiteralDictionary *useLitDict, SymbolDictionary *useSymDict, TypeDictionary *useTypeDict, uint32_t numEntryPoints, uint32_t numExitPoints, String name)
    : Allocatable()
    , _id(comp->getContextID())
    , _ext(ext)
    , _comp(comp)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, comp->mem())
    , _builders(NULL, comp->mem())
    #if 0
    , _litDict((useLitDict != NULL) ? useLitDict : comp->litdict())
    , _symDict((useSymDict != NULL) ? useSymDict : comp->symdict())
    , _typeDict((useTypeDict != NULL) ? useTypeDict : comp->typedict())
    , _numEntryPoints(numEntryPoints)
    , _numExitPoints(numExitPoints)
    #endif
    , BASECLASS_KINDINIT(kind) {

    #if 0
    initEntriesAndExits(PASSLOC, comp);
    #endif
}

Context::Context(LOCATION, ContextKind kind, Extension *ext, Context *parent, String name) //LiteralDictionary *useLitDict, SymbolDictionary *useSymDict, TypeDictionary *useTypeDict, uint32_t numEntryPoints, uint32_t numExitPoints, String name)
    : Allocatable()
    , _id(parent->comp()->getContextID())
    , _ext(ext)
    , _comp(parent->comp())
    , _name(name)
    , _parent(parent)
    , _children(NULL, _comp->mem())
    , _builders(NULL, _comp->mem())
    #if 0
    , _litDict((useLitDict != NULL) ? useLitDict : parent->litDict())
    , _symDict((useSymDict != NULL) ? useSymDict : parent->symDict())
    , _typeDict((useTypeDict != NULL) ? useTypeDict : parent->typeDict())
    , _numEntryPoints(numEntryPoints)
    , _numExitPoints(numExitPoints)
    #endif
    , BASECLASS_KINDINIT(kind) {

    parent->addChild(this);
    #if 0
    initEntriesAndExits(PASSLOC, parent->comp());
    #endif
}

#if 0
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
#endif

Context::~Context() {
    // actual Builders will be deleted by Compilation
    #if 0
    delete[] _builderExitPoints;
    delete[] _debugEntryPoints;
    delete[] _nativeEntryPoints;
    delete[] _builderEntryPoints;
    #endif
}

void
Context::addSymbol(Symbol *sym) {
    _comp->symdict()->registerSymbol(sym);
    #if 0
    if (_symDict) {
        _symDict->registerSymbol(sym);
        return;
    }

    if (_parent) {
        _parent->addSymbol(sym);
        return;
    }

    #endif
}

Symbol *
Context::lookupSymbol(String name) {
    return _comp->symdict()->LookupSymbol(name);
    #if 0
    if (_symDict) {
        sym = _symDict->LookupSymbol(name);
    }

    if (sym == NULL) {
	if (includeParents && _parent != NULL)
            return this->_parent->lookupSymbol(name);
    }
    #endif
}

} // namespace JitBuilder
} // namespace OMR
