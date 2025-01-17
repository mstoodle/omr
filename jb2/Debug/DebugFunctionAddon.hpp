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

#ifndef DEBUGFUNCTIONADDON_INCL
#define DEBUGFUNCTIONADDON_INCL

#include <map>
#include "JBCore.hpp"
#include "Func/Func.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

class Debugger;

// DebugFunctionAddon holds debugger information that can be attached to a Function
// that will be debugged. This information will be shared across all DebuggerFrames.
class DebugFunctionAddon : public Addon {
    JBALLOC_(DebugFunctionAddon)

public:
    DebugFunctionAddon(Allocator *a, Extension *ext, Func::Function *core, Debugger *jbdb, size_t valueSizeInBytes)
        : Addon(a, ext, core)
        , _jbdb(jbdb)
        , _valueSizeInBytes(valueSizeInBytes) {

    }

    size_t valueSizeInBytes() const { return _valueSizeInBytes; }

    Debugger *_jbdb;
    size_t _valueSizeInBytes;
    //ideally would hold persisted snapshot of original Compilation's IL
    Builder *_entryBuilder;

    SUBCLASS_KINDSERVICE_DECL(Extensible, DebugFunctionAddon);
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(DEBUGFUNCTIONADDON_INCL)
