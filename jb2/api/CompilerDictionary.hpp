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

#ifndef COMPILERDICTIONARY_INCL
#define COMPILERDICTIONARY_INCL

#include <map>
#include "common.hpp"
#include "Array.hpp"
#include "Compiler.hpp"
#include "CreateLoc.hpp"
#include "Extensible.hpp"
#include "Extension.hpp"
#include "List.hpp"
#include "IDs.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"

namespace OMR {
namespace JitBuilder {

class Compiler;
class Extension;
class IR;
class TextLogger;

// A second version of this class exists called IRDictionary
// Any change to this class should also likely be reflected in that class
// CompilerDictionary is a base class for Dictionaries that are managed by
// the Compiler object. They manage elements that are outside of an IR
// object so they subclass Extensible but not ExtensibleIR.

template<class EntryType, typename EntryID, EntryID NoEntry, typename EntryListType>
class CompilerDictionary : public Extensible {
    JBALLOC_NO_DESTRUCTOR_(CompilerDictionary)

    friend class Compiler;
    friend class Compilation;
    friend class Extension;
    friend class IRCloner;

public:
    CompilerDictionary(Allocator *a, DictionaryID id, Extension *ext, String name="", KINDTYPE(Extensible) kind=KIND(Extensible))
        : Extensible(a, ext, kind)
        , _id(id)
        , _name(name)
        , _compiler(ext->compiler())
        , _entries(NULL, _compiler->mem())
        , _entriesByType(NULL, _compiler->mem())
        , _nextID(NoEntry+1) {

    }

    DictionaryID id() const { return _id; }
    const String & name() const { return _name; }
    Compiler *compiler() const { return _compiler; }
    typename List<EntryType *>::Iterator iterator() const { return _entries.iterator(); }
    typename List<EntryType *>::Iterator modifiableIterator() { return _entries.iterator(true, false, false); }
    virtual void log(TextLogger &lgr) const {
        lgr.irSectionBegin("dict", "D", _id, kind(), _name);
        logContents(lgr);
        for (auto it = this->iterator();it.hasItem();it++) {
            EntryType *entry = it.item();
            entry->log(lgr);
        }
        lgr.irSectionEnd();
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
        if (it.hasItem())
            _entries.remove(it);
    }

    EntryID numEntries() const { return _nextID; }

    virtual void registerEntry(EntryType *entry) { addNewEntry(entry); }

protected:
    virtual void logContents(TextLogger &lgr) const { }

    EntryID getID() { return _nextID++; }
    void addNewEntry(EntryType *entry) {
        EntryListType *list = NULL;
        const Type *type = entry->type();
        if (_entriesByType.exists(type->id()))
            list = _entriesByType[type->id()];
        else {
            Allocator *mem = _compiler->mem();
            list = new (mem) EntryListType(mem);
            _entriesByType.assign(type->id(), list);
        }
        list->push_back(entry);
        _entries.push_back(entry);
    }

    DictionaryID _id;
    String _name;
    Compiler *_compiler;
    List<EntryType *> _entries;
    Array<EntryListType *> _entriesByType;
    EntryID _nextID;

    SUBCLASS_KINDSERVICE_DECL(Extensible, CompilerDictionary);
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(COMPILERDICTIONARY_INCL)
