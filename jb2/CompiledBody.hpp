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

#ifndef COMPILEDBODY_INCL
#define COMPILEDBODY_INCL

#include "common.hpp"
#include "Array.hpp"
#include "CreateLoc.hpp"
#include "IDs.hpp"
#include "NativeEntry.hpp"

namespace OMR {
namespace JB2 {

class CompileUnit;
class Context;

class CompiledBody : public Allocatable {
    JBALLOC(CompiledBody, NoAllocationCategory)

    friend class Compiler;

public:
    DYNAMIC_ALLOC_ONLY(CompiledBody, CompileUnit *unit, StrategyID strategy);

    CompiledBodyID id() const { return _id; }
    CompileUnit *unit() const { return _unit; }
    StrategyID strategy() const { return _strategy; }
    CompilerReturnCode rc() const { return _rc; }

    template<typename T>
    T *nativeEntryPoint(unsigned e=0) const {
        assert(e < _nativeEntries.length());
        return _nativeEntries[e]->entry<T>();
    }
    void addNativeEntry(NativeEntry *e) {
        EntryID id = e->entryID();
        if (id < _nativeEntries.length())
            assert(_nativeEntries[id] == NULL);
        _nativeEntries.assign(id, e);
    }

    template<typename T>
    T *debugEntryPoint(unsigned e=0) const {
        assert(e < _debugEntries.length());
        return _debugEntries[e]->entry<T>();
    }
    void addDebugEntry(NativeEntry *e) {
        EntryID id = e->entryID();
        if (id < _debugEntries.length())
            assert(_debugEntries[id] == NULL);
        _debugEntries.assign(id, e);
    }

protected:
    CompiledBody * setReturnCode(CompilerReturnCode rc) { _rc = rc; return this; }

    CompiledBodyID _id;
    CompileUnit *_unit;
    StrategyID _strategy;
    CompilerReturnCode _rc;
    Array<NativeEntry *> _nativeEntries;
    Array<NativeEntry *> _debugEntries;
};

} // namespace JB2
} // namespace OMR

#endif // defined(COMPILEDBODY_INCL)

