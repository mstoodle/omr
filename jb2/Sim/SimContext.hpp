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

#ifndef DEBUGGERFRAME_INCL
#define DEBUGGERFRAME_INCL

#include <stdint.h>
#include <map>
#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

class Breakpoint;
class Debugger;
class DebugValue;
class FunctionDebugInfo;

// This structure represents the Debugger state for each activation on a thread stack.
// Because the generated code accesses fields of this struct, we consciously use a
// 'struct' rather than a class and it should never contain any virtual functions or
// be subclassed (should remain POD).

struct DebuggerFrame {
    FunctionDebugInfo * _info;
    Debugger *_debugger;
    DebugValue *_locals; // array of values, but size of each determined by _DebugValue->size()
    DebugValue *_values; // array of values, but size of each determined by _DebugValue->size()
    DebugValue *_returnValues; // array of values, but size of each determined by _DebugValule->size()
    Builder * _fromBuilder;
    bool      _returning;
    Builder * _builderToDebug;

    std::map<uint64_t, Operation *> _reentryPoints;
    List<Breakpoint *> _breakpoints;

    DebugValue *getValueInArray(uint8_t *p, uint64_t idx);
    DebugValue *getValue(uint64_t idx);
    DebugValue *getLocal(uint64_t idx);
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(DEBUGGERFRAME_INCL)
