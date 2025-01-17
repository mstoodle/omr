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

#ifndef SCOPE_INCL
#define SCOPE_INCL

#include "ExtensibleIR.hpp"
#include "List.hpp"
#include "String.hpp"

// Scope is used to organize set of Builders that are naturally associated with one
// another. For example, the Builders from one Function may be collected into a
// FunctionScope. When one Function is inlined into another, the Builders of the
// inner Function will have a different FunctionScope than the Builders of the outer
// Function, yet the inner FunctionScope will have the outer FunctionScope as its parent.
// A Scope can have a specific Context or, if it has a NULL Context, inherits from its
// parent Scope (or the Compilation, though typically there is a Scope associated with
// Compilation that would be the (inherited) parent for all Scopes inside that
// CompileUnit. Note that Scopes are created and destroyed during a Compilation: they
// do not exist outside the lifecycle of a Compilation, whereas it is possible for a
// Context to exist outside of a Compilation (i.e. tied to a Compiler's lifecycle).
// Contexts represent "data" as well as dictionaries to access that data. Scopes are
// code regions that have access to those Contexts.
//
// Scopes are originally created based mostly upon either language-visible state (e.g.
// local variables) or runtime-visible (e.g. lamdda implementation) state associated with
// what the code in the CompileUnit can access. Scopes may, however, be refined (created
// to refer to subgraphs of an original Scope's Builders) during the process of
// Compilation. For example, imagine a register allocation pass that may create a
// "RegisterContext" and a Scope to access it that applies to a Builder or subset of
// Builders that were not an original Scope created when the Compilation began.
// Alternatively, a "shrink wrapping" pass could create multiple overlapping Contexts
// with overlapping (but not necessarily mutually exclusive) Scopes to reduce the live
// ranges for local variables.
//
// Scopes can therefore change throughout Compilation and are also not guaranteed to
// "nest" in a structured way, although it's expected that mostly Scopes will have
// a relatively simple nested structure because that's how most languages define
// scoping rules for both data and code. Scopes have entry points and exit points
// and these can also change throughout a Compilation. For example, a FunctionScope
// can acquire a new entry point after inlining another Function that contains a
// "yield" operation (where the new entry point would be specifically used for
// continuing from the "yield" as opposed to calling the Function anew).

#include "Context.hpp"
#include "EntryPoint.hpp"

namespace OMR {
namespace JB2 {

class Compilation;
class CompiledBody;
class Extension;
class IR;

class Scope : public ExtensibleIR {
    JBALLOC_(Scope)

    friend class Compilation;
    friend class IRCloner;

public:
    DYNAMIC_ALLOC_ONLY(Scope, Extension *ext, IR *ir, String name="");
    DYNAMIC_ALLOC_ONLY(Scope, Extension *ext, Scope *parent, String name="");

    ScopeID id() const { return _id; }
    const String & name() const { return _name; }
    IR *ir() const { return _ir; }

    void log(TextLogger & lgr) const;

    void addInitialBuildersToWorklist(BuilderList & worklist);

    // Entry point handling
    template <class T>
    T *entryPoint(EntryID e=0) const {
        EntryPoint *ep = findEntryPoint(e, CLASSKIND(T,Extensible));
        return ep->refine<T>();
    }
    template <class T>
    size_t numEntryPoints() {
        return _entries.length();
    }
    void addEntryPoint(EntryPoint *entry, EntryID e=0);

    // Exit handling
    virtual uint32_t numExits() const { return 0; }
    virtual Builder *exit(uint32_t x=0) const { return NULL; }
    virtual void addExit(Builder *b, uint32_t x=0) { assert(0); }

    virtual BuilderListIterator builderIterator() const { return _allBuilders.fwdIterator(); }

    virtual Builder *transfer(Builder *fromBuilder, Builder *toBuilder);

    virtual void saveEntries(CompiledBody *body);

protected:
    DYNAMIC_ALLOC_ONLY(Scope, Extension *ext, KINDTYPE(Extensible) kind, IR *ir, String name="");
    DYNAMIC_ALLOC_ONLY(Scope, Extension *ext, KINDTYPE(Extensible) kind, Scope *parent, String name="");
    Scope(Allocator *a, const Scope *source, IRCloner *cloner);

    virtual ExtensibleIR *clone(Allocator *mem, IRCloner *cloner) const { return cloneScope(mem, cloner); }
    virtual Scope *cloneScope(Allocator *mem, IRCloner *cloner) const;

    virtual void logContents(TextLogger &lgr)  { }

    EntryPoint *findEntryPoint(EntryID e, ExtensibleKind kind) const;
    void addChild(Scope *child) { _children.push_back(child); }
    void addBuilder(Builder *b) { _allBuilders.push_back(b); }

    virtual Builder * enter(Builder *fromBuilder, Builder *toBuilder) {
        // what must be done to enter this scope on the way to toBuilder
        return toBuilder;
    }

    virtual Builder * exit(Builder *fromBuilder, Builder *toBuilder) {
        // what must be done to exit this scope from fromBuilder
        return toBuilder;
    }

    uint64_t _id;
    IR *_ir;
    String _name;

    Scope * _parent;
    List<Scope *> _children;
    List<Builder *> _allBuilders;
    Array<List<EntryPoint *> *> _entries;

    SUBCLASS_KINDSERVICE_DECL(Extensible, Scope);
};

} // namespace JB2
} // namespace OMR

#endif // defined(SCOPE_INCL)
