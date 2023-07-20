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
#include "CompiledBody.hpp"
#include "Scope.hpp"


namespace OMR {
namespace JitBuilder {

INIT_JBALLOC(Scope)
BASECLASS_KINDSERVICE_IMPL(Scope)

Scope::Scope(ScopeKind kind, Extension *ext, Compilation *comp, String name)
    : Allocatable()
    , _id(comp->getScopeID())
    , _ext(ext)
    , _comp(comp)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, comp->mem())
    , _entries(NULL, comp->mem())
    , _allBuilders(NULL, comp->mem())
    , BASECLASS_KINDINIT(kind) {

}

Scope::Scope(Allocator *a, ScopeKind kind, Extension *ext, Compilation *comp, String name)
    : Allocatable(a)
    , _id(comp->getScopeID())
    , _ext(ext)
    , _comp(comp)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, comp->mem())
    , _entries(NULL, comp->mem())
    , _allBuilders(NULL, comp->mem())
    , BASECLASS_KINDINIT(kind) {

}

Scope::Scope(ScopeKind kind, Extension *ext, Scope *parent, String name)
    : Allocatable()
    , _id(parent->comp()->getScopeID())
    , _ext(ext)
    , _comp(parent->comp())
    , _name(name)
    , _parent(parent)
    , _children(NULL, _comp->mem())
    , _entries(NULL, _comp->mem())
    , _allBuilders(NULL, _comp->mem())
    , BASECLASS_KINDINIT(kind) {

    parent->addChild(this);
}

Scope::Scope(Allocator *a, ScopeKind kind, Extension *ext, Scope *parent, String name)
    : Allocatable(a)
    , _id(parent->comp()->getScopeID())
    , _ext(ext)
    , _comp(parent->comp())
    , _name(name)
    , _parent(parent)
    , _children(NULL, _comp->mem())
    , _entries(NULL, _comp->mem())
    , _allBuilders(NULL, _comp->mem())
    , BASECLASS_KINDINIT(kind) {

    parent->addChild(this);
}

Scope::~Scope() {

}

void
Scope::addEntryPoint(EntryPoint *entry, EntryID e) {
    List<EntryPoint *> *list = NULL;
    if (e < _entries.length()) {
        list = _entries[e];
    } else {
        Allocator *mem = _comp->mem();
        list = new (mem) List<EntryPoint *>(mem, mem);
        _entries.assign(e, list);
    }
    list->push_back(entry);
}

EntryPoint *
Scope::findEntryPoint(EntryID e, EntryPointKind kind) const {
    List<EntryPoint *> *list = _entries[e];
    if (list != NULL) {
        for (auto it=list->iterator(); it.hasItem(); it++) {
            EntryPoint *entry = it.item();
            if (entry->kind() == kind) {
                return entry;
            }
        }
    }
    return NULL;
}

Builder *
Scope::transfer(Builder *fromBuilder, Builder *toBuilder) {
    if (fromBuilder->scope() == toBuilder->scope())
        return toBuilder;
    Builder *transition = fromBuilder->scope()->exit(fromBuilder, toBuilder);
    toBuilder->scope()->enter(transition, toBuilder);
    return transition;
}

void
Scope::saveEntries(CompiledBody *body) {
    for (auto e=0;e < _entries.length();e++) {
        List<EntryPoint *> *list = _entries[e];
        if (list != NULL) {
            for (auto it=list->iterator(); it.hasItem(); it++) {
                EntryPoint *entry = it.item();
                if (entry->isKind<NativeEntry>()) {
                    NativeEntry *ne = entry->refine<NativeEntry>();
                    body->addNativeEntry(ne);
                }
            }
        }
    }
}

} // namespace JitBuilder
} // namespace OMR
