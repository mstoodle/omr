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
#include "IRCloner.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"


namespace OMR {
namespace JB2 {

INIT_JBALLOC(Context)
SUBCLASS_KINDSERVICE_IMPL(Context, "Context", ExtensibleIR, Extensible)

Context::Context(Allocator *a, Extension *ext, IR *ir, String name)
    : ExtensibleIR(a, ext, ir, CLASSKIND(Context, Extensible))
    , _id(ir->getContextID())
    , _ir(ir)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, ir->mem()) {

    ir->setContext(this);
}

Context::Context(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind, IR *ir, String name)
    : ExtensibleIR(a, ext, ir, kind)
    , _id(ir->getContextID())
    , _ir(ir)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, ir->mem()) {

    ir->setContext(this);
}

Context::Context(Allocator *a, Extension *ext, Context *parent, String name)
    : ExtensibleIR(a, ext, parent->ir(), CLASSKIND(Context, Extensible))
    , _id(parent->ir()->getContextID())
    , _ir(parent->ir())
    , _name(name)
    , _parent(parent)
    , _children(NULL, _ir->mem()) {

    parent->addChild(this);
}

Context::Context(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind, Context *parent, String name)
    : ExtensibleIR(a, ext, parent->ir(), kind)
    , _id(parent->ir()->getContextID())
    , _ir(parent->ir())
    , _name(name)
    , _parent(parent)
    , _children(NULL, _ir->mem()) {

    parent->addChild(this);
}

Context::Context(Allocator *a, const Context *source, IRCloner *cloner)
    : ExtensibleIR(a, source->ext(), cloner->clonedIR(), source->kind())
    , _id(source->_id)
    , _ir(cloner->clonedIR())
    , _name(source->_name)
    , _parent(source->_parent)
    , _children(NULL, a) {


    for (auto it=_children.iterator();it.hasItem();it++) {
        Context *child = it.item();
        _children.push_back(cloner->clonedContext(child));
    }
}

Context *
Context::cloneContext(Allocator *a, IRCloner *cloner) const {
    return new (a) Context(a, this, cloner);
}

void
Context::log(TextLogger &lgr) const {
    lgr.irSectionBegin("context", "x", id(), kind(), name());
    logContents(lgr);
    lgr.irSectionEnd();
    for (auto it=_children.iterator();it.hasItem();it++) {
        Context *child = it.item();
        child->log(lgr);
    }
}

void
Context::logContents(TextLogger &lgr) const {
    if (_parent == NULL)
        lgr.irFlagBegin("parent NULL");
    else {
        lgr.irFlagBegin("parent") << _parent;
    }
    lgr.irFlagEnd();
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
}

void
Context::addSymbol(Symbol *sym) {
    _ir->symdict()->addNewEntry(sym);
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
    return _ir->symdict()->Lookup(name);
    #if 0
    if (_symDict) {
        sym = _symDict->Lookup(name);
    }

    if (sym == NULL) {
	if (includeParents && _parent != NULL)
            return this->_parent->lookupSymbol(name);
    }
    #endif
}

} // namespace JB2
} // namespace OMR
