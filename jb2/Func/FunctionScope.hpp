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

#ifndef FUNCTIONSCOPE_INCL
#define FUNCTIONSCOPE_INCL

#include "JBCore.hpp"

// Context is an extremely important concept. Every Builder B has a Context C though B
// can (and frequently does) reuse B's parent's Context. A Builder B's Context C includes
// any bound Builders referenced by B's Operations; Contexts nest via bound Builders. So,
// for example, the Context for an outer loop includes an inner loop (though the inner
// loop and the outer loop can have different Contexts, the inner loop's Context will have
// the outer loop's Context as its parent). But an operation O that conditionally branches
// to another Builder does not necessarily have (though it can have) the same Context as
// O's parent. The CompileUnit has a single Context that covers all the Builders in the
// CompileUnit. This special Context has designated exit Builders for each program point
// that exits the CompileUnit. In the common case of a function, there would be one
// designated exit Builder corresponding to each program point that returns from the
// function. The return itself takes place from the exit Builder; the program point ends
// with an unconditional Goto operation to the exit Builder. The exit Builder can contain
// whatever Operations are needed to perform the exit.
//
// A Context can in general have multiple entry points (Builders) and transfer destinations
// (Builders) but frequently has a single entry point and may also have a single transfer
// destination. Transfer destinations are simply Builders that are not contained in the
// Context to which an Operation in the Context can direct control flow. Every Context but
// the CompileUnit's Context has a parent Context. In addition, each Context may optionally
// contain a LiteralDictionary, a SymbolDictionary, and/or a TypeDictionary. If a Context is
// not created with a specific *Dictionary of each kind, lookups for that kind (Literals,
// Symbols, or Types) automatically delegate to the parent Context. Delegation can be
// prevented, if desired, by providing specific *Dictionaries that do not have parent
// Dictionary objects. Note that the various Dictionary objects will also delegate lookup to
// their parent Dictionary if specified at creation.
//
// Contexts are the primary way that scoping is represented in the code (especially when the
// initial IL is produced), but different subclasses of Contexts can facilitate closures, or
// to represent different kinds of informaton propagating through a Builder's Operations
// (Contexts play a central role with OperationRewriter, for example).
//
// Contexts can be windows to different kinds of state:
//    - stack frame
//    - thread local data
//    - global memory
//    - tenant context
//    - debug stack frame or debug thread local data or debug memory
//    - register state
//    - some kinds of privatized variables?
//    - mock state?
//    - profiled state?
// One accesses each of these windows through Symbols. Contexts can be built on other Contexts.
// For example, a debug stack frame may be built on top of a stack frame context but provide
// a mapping between the Symbols loaded through the debug Context to different Symbols loaded
// through the underlying stack frame.
//
// Need an Executor to provide same mechanism for Builders and Operations?
//    - Direct just runs it directly or transfers directly to the same Builder object
//    - DebugExecutor runs through DebugContexts?
//    - Folder runs with LiteralContexts and identies control flows as definite / definitely not / indeterminate and propagates Literals to results
//    - Enclave manages transitions across Context boundaries
//    - Profiler collects information about paths/symbols/values
//
// Maybe Context needs to be split into Context and Scope?
//    - Context is a data context that changes over time
//    - Scope is a code context with entry points and exits?
//        - CompileUnit is particular kind of a Scope
//    - Executor handles execution for a Scope given a particular Context
//
// Contexts are a therefore critical infrastructure for the Compiler to analyze, transform,
// and manage the compilation process.

namespace OMR {
namespace JB2 {
namespace Func {

class FunctionScope : public Scope {
    JBALLOC_(FunctionScope)

public:
    DYNAMIC_ALLOC_ONLY(FunctionScope, Extension *ext, IR *ir, String name="");
    DYNAMIC_ALLOC_ONLY(FunctionScope, Extension *ext, Scope *parent, String name="");

protected:
    FunctionScope(Allocator *a, const FunctionScope *source, IRCloner *cloner);

    virtual Scope *clone(Allocator *mem, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Extensible, FunctionScope);
};

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // defined(FUNCTIONSCOPE_INCL)
