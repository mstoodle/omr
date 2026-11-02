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

#ifndef DICTIONARY_INCL
#define DICTIONARY_INCL

#include "common.hpp"
#include "Array.hpp"
#include "Compiler.hpp"
#include "CreateLoc.hpp"
#include "ExtensibleIR.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"
#include "List.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"

namespace OMR {
namespace JB2 {

class Extension;
class TextLogger;

// This Base class may not be used directly by many consumers outside of its subclasses, but it helps
// to share some common elements across all kinds of dictionaries
// BUT it is a friend of IR so that it can fetch the next DictionaryID
class BaseDictionary : public ExtensibleIR {
    JBALLOC_NO_DESTRUCTOR_(BaseDictionary)

public:
    DictionaryID id() const { return _id; }
    String name() const { return _name; }

protected:
    BaseDictionary(Allocator *a, Extension *ext, IR *ir, String name=String(""), KINDTYPE(Extensible) kind=KIND(Extensible))
        : ExtensibleIR(a, ext, ir, kind)
        , _id(ir->getDictionaryID())
        , _name(name) {
    }
    BaseDictionary(Allocator *a, const BaseDictionary *source, IRCloner *cloner)
        : ExtensibleIR(a, source->ext(), cloner->clonedIR(), source->kind())
        , _id(source->_id)
        , _name("") {

    }

    DictionaryID _id;
    String _name;
};

template<class EntryType, typename EntryID, EntryID NoEntry, typename EntryListType>
class Dictionary : public BaseDictionary {
    JBALLOC_NO_DESTRUCTOR_(Dictionary)

    friend class Compiler;
    friend class Compilation;
    friend class Extension;
    friend class IR;
    friend class IRCloner;

public:
    Dictionary(Allocator *a, Extension *ext, IR *ir, String name="", KINDTYPE(Extensible) kind=KIND(Extensible))
        : BaseDictionary(a, ext, ir, name, kind)
        , _numEntries(0)
        , _entries(NULL, ir->mem())
        , _entriesByType(NULL, ir->mem()) {

    }

    virtual ~Dictionary() {
        #if 0
        for (auto it = iterator(); it.hasItem(); it++) {
            EntryType *entry = it.item();
            delete entry;
        }
        #endif
    }

    EntryID numEntries() const { return _numEntries; }

    typename List<EntryType *>::Iterator iterator(bool fwd=true, bool detectChanges=true, bool makeCopy=false) const { return _entries.iterator(fwd, detectChanges, makeCopy); }
    virtual void log(TextLogger &lgr) const {
        if (_entries.empty()) {
            lgr.irSectionOneLiner("dictionary", "D", id(), kind(), name());
        } else {
            lgr.irSectionBegin("dictionary", "D", id(), kind(), name());
            logContents(lgr);
            for (auto it = this->iterator();it.hasItem();it++) {
                EntryType *entry = it.item();
                entry->log(lgr, true);
            }
            lgr.irSectionEnd();
        }
    }

    virtual void addNewEntry(EntryType *entry) {
        EntryListType *list = NULL;
        const Type *type = entry->type();
        TypeID id = type->id();
        if (_entriesByType.exists(id) && _entriesByType[id] != NULL)
            list = _entriesByType[type->id()];
        else {
            Allocator *mem = ir()->mem();
            list = new (mem) EntryListType(mem);
            _entriesByType.assign(type->id(), list);
        }
        list->push_back(entry);
        _entries.push_back(entry);
        _numEntries++;
    }

    EntryType *Lookup(EntryID id) {
        for (auto it = iterator(); it.hasItem(); it++) {
            EntryType *entry = it.item();
            if (entry->id() == id)
                return entry;
        }
        return NULL;
    }

    void Remove(EntryType *entry) {
        auto it = _entries.find(entry);
        if (it.hasItem()) {
            _entries.remove(it);
            _entriesByType.remove(entry->id());
            _numEntries--;
        }
    }

protected:
    Dictionary(Allocator *a, const Dictionary *source, IRCloner *cloner)
        : BaseDictionary(a, source, cloner)
        , _numEntries(0)
        , _entries(NULL, ir()->mem())
        , _entriesByType(NULL, ir()->mem()) {
    
        // expectation is that IR will call cloneFrom() once this object has been created
        // which prevents recursive cloning into the dictionary objects while processing entries
    }

    // must be called after this dictionary has been constructed
    void cloneFrom(const Dictionary<EntryType, EntryID, NoEntry, EntryListType> *source, IRCloner *cloner) {
        for (auto it = source->iterator(); it.hasItem(); it++) {
            EntryType *entry = it.item();
            EntryID id = entry->id();
            // entry may have been cloned already via a link in an earlier entry, don't re-clone in that case
            if (!_entriesByType.exists(id) || _entriesByType[id] == NULL) {
                EntryType *clonedEntry = cloner->clone(entry)->template refine<EntryType>();
                addNewEntry(clonedEntry);
            }
        }
    }
    // must be called after this dictionary has been constructed
    void cloneConstFrom(const Dictionary<EntryType, EntryID, NoEntry, EntryListType> *source, IRCloner *cloner) {
        for (auto it = source->iterator(); it.hasItem(); it++) {
            EntryType *entry = it.item();
            EntryID id = entry->id();
            // entry may have been cloned already via a link in an earlier entry, don't re-clone in that case
            if (!_entriesByType.exists(id) || _entriesByType[id] == NULL) {
                EntryType *clonedEntry = cloner->cloneConst(entry)->template refine<EntryType>();
                addNewEntry(clonedEntry);
            }
        }
    }

    virtual void logContents(TextLogger &lgr) const { }

    EntryID _numEntries;
    List<EntryType *> _entries;
    Array<EntryListType *> _entriesByType;

    SUBCLASS_KINDSERVICE_DECL(Extensible, Dictionary);
};

} // namespace JB2
} // namespace OMR

#endif // defined(DICTIONARY_INCL)
