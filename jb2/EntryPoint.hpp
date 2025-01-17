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

#ifndef ENTRYPOINT_INCL
#define ENTRYPOINT_INCL

#include "ExtensibleIR.hpp"
#include "List.hpp"
#include "String.hpp"


namespace OMR {
namespace JB2 {

class Compilation;
class IR;
class IRCloner;
class TextLogger;

class EntryPoint : public ExtensibleIR {
    JBALLOC_(EntryPoint)

    friend class IRCloner;

public:
    DYNAMIC_ALLOC_ONLY(EntryPoint, IR *ir, ExtensibleKind kind, EntryID entryID, String name="");

    EntryPointID id() const { return _id; }
    EntryID entryID() const { return _entryID; }
    bool isEntry(EntryID entryID) { return _entryID == entryID; }

    const String & name() const { return _name; }

    void log(TextLogger & lgr) const;

protected:
    EntryPoint(Allocator *a, const EntryPoint *source, IRCloner *cloner);
    virtual ExtensibleIR *clone(Allocator *mem, IRCloner *cloner) const { return cloneEntryPoint(mem, cloner); }
    virtual EntryPoint *cloneEntryPoint(Allocator *mem, IRCloner *cloner) const;
    virtual void logContents(TextLogger & lgr) const { }

    EntryPointID _id;
    EntryID _entryID;
    IR *_ir;
    String _name;

    SUBCLASS_KINDSERVICE_DECL(Extensible, EntryPoint);
};

} // namespace JB2
} // namespace OMR

#endif // defined(ENTRYPOINT_INCL)
