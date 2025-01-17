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

#include "BitVector.hpp"
#include "Builder.hpp"
#include "BuilderEntry.hpp"
#include "Compilation.hpp"
#include "CompiledBody.hpp"
#include "IRCloner.hpp"
#include "Operation.hpp"
#include "Scope.hpp"
#include "TextLogger.hpp"


namespace OMR {
namespace JB2 {

INIT_JBALLOC(Scope)
SUBCLASS_KINDSERVICE_IMPL(Scope, "Scope", Extensible, Extensible)

Scope::Scope(Allocator *a, Extension *ext, IR *ir, String name)
    : ExtensibleIR(a, ext, ir, KIND(Extensible))
    , _id(ir->getScopeID())
    , _ir(ir)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, ir->mem())
    , _entries(NULL, ir->mem())
    , _allBuilders(NULL, ir->mem()) {

    _ir->setScope(this);
}

Scope::Scope(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind, IR *ir, String name)
    : ExtensibleIR(a, ext, ir, kind)
    , _id(ir->getScopeID())
    , _ir(ir)
    , _name(name)
    , _parent(NULL)
    , _children(NULL, ir->mem())
    , _entries(NULL, ir->mem())
    , _allBuilders(NULL, ir->mem()) {

    _ir->setScope(this);
}

Scope::Scope(Allocator *a, Extension *ext, Scope *parent, String name)
    : ExtensibleIR(a, ext, parent->ir(), KIND(Extensible))
    , _id(parent->ir()->getScopeID())
    , _ir(parent->ir())
    , _name(name)
    , _parent(parent)
    , _children(NULL, _ir->mem())
    , _entries(NULL, _ir->mem())
    , _allBuilders(NULL, _ir->mem()) {

    parent->addChild(this);
}

Scope::Scope(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind, Scope *parent, String name)
    : ExtensibleIR(a, ext, parent->ir(), kind)
    , _id(parent->ir()->getScopeID())
    , _ir(parent->ir())
    , _name(name)
    , _parent(parent)
    , _children(NULL, _ir->mem())
    , _entries(NULL, _ir->mem())
    , _allBuilders(NULL, _ir->mem()) {

    parent->addChild(this);
}

Scope::Scope(Allocator *a, const Scope *source, IRCloner *cloner)
    : ExtensibleIR(a, source->ext(), cloner->clonedIR(), source->_kind)
    , _id(source->_id)
    , _ir(cloner->clonedIR())
    , _name(source->_name)
    , _parent(cloner->clonedScope(source->_parent))
    , _children(NULL, a)
    , _entries(NULL, a)
    , _allBuilders(NULL, a) {

    // children
    for (auto it=source->_children.iterator(); it.hasItem(); it++) {
        Scope *child = it.item();
        _children.push_back(cloner->clonedScope(child));
    }

    // entries
    for (uint32_t i=0; i < source->_entries.length();i++) {
        List<EntryPoint *> *list = source->_entries[i];
        if (list != NULL) {
            List<EntryPoint *> *cloned_list = new (a) List<EntryPoint *>(a, a);
            for (auto it2 = list->iterator();it2.hasItem();it2++) {
                EntryPoint *ep = it2.item();
                EntryPoint *cloned_ep = cloner->clonedEntryPoint(ep);
                if (cloned_ep != NULL)
                    cloned_list->push_back(cloned_ep);
            }
            if (cloned_list->length() > 0)
                _entries.assign(i, cloned_list);
            else
                delete cloned_list;
        }
    }

    // builders
    for (auto it=source->_allBuilders.iterator(); it.hasItem(); it++) {
        Builder *b = it.item();
        _allBuilders.push_back(cloner->clonedBuilder(b));
    }
}

Scope *
Scope::cloneScope(Allocator *mem, IRCloner *cloner) const {
    return new (mem) Scope(mem, this, cloner);
}

Scope::~Scope() {
    for (auto it = _entries.iterator();it.hasItem();it++) {
        List<EntryPoint *> *list = it.item();
        if (list != NULL) {
            for (auto it2 = list->iterator();it2.hasItem();it2++) {
                EntryPoint *ep = it2.item();
                // NativeEntry is transferred to CompiledBody
                if (!ep->isKind<NativeEntry>())
                    delete ep;
            }
            delete list;
        }
    }
}

void
Scope::addInitialBuildersToWorklist(BuilderList & worklist) {
    for (uint32_t e=0;e < _entries.length();e++) {
        Builder *b = entryPoint<BuilderEntry>(e)->builder();
        worklist.push_back(b);
    }
}

void
Scope::addEntryPoint(EntryPoint *entry, EntryID e) {
    List<EntryPoint *> *list = NULL;
    if (e < _entries.length()) {
        list = _entries[e];
    } else {
        Allocator *mem = _ir->mem();
        list = new (mem) List<EntryPoint *>(mem, mem);
        _entries.assign(e, list);
    }
    list->push_back(entry);
}

EntryPoint *
Scope::findEntryPoint(EntryID e, ExtensibleKind kind) const {
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

void
Scope::log(TextLogger & lgr) const {
    lgr.irSectionBegin("scope", "Scope", id(), kind(), name());
    lgr.irFlagBegin("entries") << _entries.length() << lgr.endl();
    lgr.indentIn();
    for (EntryID e=0;e < _entries.length();e++) {
        List<EntryPoint *> *epList = _entries[e];
        if (epList != NULL) {
            for (auto it=epList->iterator(); it.hasItem(); it++) {
                EntryPoint *entry = it.item();
                entry->log(lgr);
            }
        }
    }
    lgr.indentOut();
    lgr.indent(); lgr.irFlagEnd();

    {
        Allocator *mem = _ir->mem();
        BuilderList worklist(NULL, mem);
        BitVector visited(mem, _ir->maxBuilderID());
        _ir->addInitialBuildersToWorklist(worklist);

        while(!worklist.empty()) {
            Builder *b = worklist.back();
            int64_t id = b->id();
            if (visited.getBit(id)) {
                worklist.pop_back();
                continue;
            }
            visited.setBit(id);
            b->logPrefix(lgr);
            for (Operation *op = b->firstOperation(); op != NULL; op = op->next()) {
                op->logFull(lgr);

                for (auto it = op->builders(); it.hasItem(); it++) {
                    Builder * inner_b = it.item();
                    if (inner_b && !visited.getBit(inner_b->id())) {
                        worklist.push_front(inner_b);
                    }
                }
            }
            b->logSuffix(lgr);
            worklist.pop_back();
        }
    }

    lgr.irSectionEnd();
}

} // namespace JB2
} // namespace OMR
