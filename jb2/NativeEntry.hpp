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

#ifndef NATIVEENTRY_INCL
#define NATIVEENTRY_INCL

#include "EntryPoint.hpp"


namespace OMR {
namespace JB2 {

class Compilation;

class NativeEntry : public EntryPoint {
    JBALLOC_(NativeEntry)

public:
    DYNAMIC_ALLOC_ONLY(NativeEntry, IR *ir, EntryID id, void *entry, String name="");

    template<typename T>
    T *entry() { return reinterpret_cast<T *>(_entry); }

protected:
    DYNAMIC_ALLOC_ONLY(NativeEntry, IR *ir, ExtensibleKind kind, EntryID id, void *entry, String name="");
    void logContents(TextLogger & lgr) const;

    void *_entry;

    SUBCLASS_KINDSERVICE_DECL(Extensible,NativeEntry);
};

} // namespace JB2
} // namespace OMR

#endif // defined(NATIVEENTRY_INCL)
