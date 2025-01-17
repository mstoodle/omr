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

#ifndef DEBUGVALUE_INCL
#define DEBUGVALUE_INCL

namespace OMR {
namespace JB2 {
namespace Debug {

// DebugValue is used to record concrete values and locals computed by the debugged functions
// The possible values that can be stored here are determined at runtime per Function based
// on the variety of Types that are manipulated by the Function (as stored in its TypeDictionary).
// This type as defined is just a prefix for the actual dynamic type: the payload of the type is
// determined dynamically per Function. Allocating and indexing into arrays of debug values will
// use the size of the DebugDictionary's _DebugValue type which takes into account all these Types.
// Because it's a union, the address of the _firstValueData field is used as the offset where any
// actual value is stored, regardless of its Type.

class DebugValue {
public:
    Type *_type;
    // conceptually followed by:
    // union {
    //   one field per type defined in the Function's TypeDictionary
    // };

    // dummy field: not used by anything other than to locate the start of the conceptual union described above
    uintptr_t _firstValueData;

    // to allocate space for a DebugValue, you have to ask a specific Function's DebugDictionary for the size
    // of its _DebugValue type.  Note that different Functions have different corresponding _DebugValue types
    // because different Functions access different sets of Types.
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(DEBUGVALUE_INCL)
