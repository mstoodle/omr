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

#ifndef IRDICTIONARY_INCL
#define IRDICTIONARY_INCL

#include <map>
#include "common.hpp"
#include "Array.hpp"
#include "Compiler.hpp"
#include "CreateLoc.hpp"
#include "ExtensibleIR.hpp"
#include "IR.hpp"
#include "List.hpp"
#include "Type.hpp"

namespace OMR {
namespace JitBuilder {

class Compiler;
class Extension;
class IR;
class TextLogger;

// A second version of this class exists called CompilerDictionary
// Any change to this class should also likely be reflected in that class
// IRDictionary is a base class for Dictionaries that are managed inside
// the IR object and so must subclass from ExtensibleIR

template<class EntryType, typename EntryID, EntryID NoEntry, typename EntryListType>
class IRDictionary : public ExtensibleIR {
    JBALLOC_NO_DESTRUCTOR_(IRDictionary)

    friend class Compiler;
    friend class Compilation;
    friend class Extension;
    friend class IR;
    friend class IRCloner;

public:
    IRDictionary(Allocator *a, DictionaryID id, Extension *ext, IR *ir, String name="", KINDTYPE(Extensible) kind=KIND(Extensible))
        : ExtensibleIR(a, ext, ir, kind)
        , _id(id)
        , _name(name)
        , _entries(NULL, ir->mem())
        , _entriesByType(NULL, ir->mem())
        , _nextID(NoEntry+1) {

    }

    DictionaryID id() const { return _id; }
    const String & name() const { return _name; }
    typename List<EntryType *>::Iterator iterator() const { return _entries.iterator(); }
    virtual void log(TextLogger &lgr) const { }

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

protected:
    IRDictionary(Allocator *a, const IRDictionary *source, IRCloner *cloner)
        : ExtensibleIR(a, source->ext(), source->ir(), source->kind())
        , _id(source->_id)
        , _name(source->_name)
        , _entries(NULL, ir()->mem())
        , _entriesByType(NULL, ir()->mem())
        , _nextID(source->_nextID) {
    
        // relies on subclass to clone entries appropriately
    }
    virtual void logContents(TextLogger &lgr) const { }

    EntryID getID() { return _nextID++; }
    void addNewEntry(EntryType *entry) {
        EntryListType *list = NULL;
        const Type *type = entry->type();
        if (_entriesByType.exists(type->id()))
            list = _entriesByType[type->id()];
        else {
            Allocator *mem = ir()->mem();
            list = new (mem) EntryListType(mem);
            _entriesByType.assign(type->id(), list);
        }
        list->push_back(entry);
        _entries.push_back(entry);
    }

    DictionaryID _id;
    String _name;
    List<EntryType *> _entries;
    Array<EntryListType *> _entriesByType;
    EntryID _nextID;

    SUBCLASS_KINDSERVICE_DECL(Extensible, IRDictionary);
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(IRDICTIONARY_INCL)
