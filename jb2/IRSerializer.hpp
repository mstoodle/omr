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

#ifndef IRSERIALIZER_INCL
#define IRSERIALIZER_INCL

#include <cstddef>
#include <cstdint>
#include "Extensible.hpp"
#include "IDSpaceService.hpp"
#include "IDs.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// IRSerializer is the abstract base class for all IR serialization strategies.
// It is a pure stream-writing primitive — it has no knowledge of IR elements.
//
// IR elements call back into this interface to emit their constituent fields.
// IR::write(IRSerializer &) drives the overall serialization process, calling
// element->write(s) for full detail and element->writeRef(s) for cross-
// references.  IRSerializer itself never references any IR element type.
//
// Class hierarchy:
//
//   IRSerializer                      (this file — abstract)
//     ├── TextSerializer              (TextSerializer.hpp — human-readable text)
//     └── BinarySerializer            (BinarySerializer.hpp — binary mid-layer, abstract)
//           └── BasicBinarySerializer (BasicBinarySerializer.hpp — raw fixed-width encoding)
//
// writeID() is the single extensible virtual for all IR identifier spaces.
// It takes an IDSpaceID — a compact integer registered with IDSpaceService —
// so subclasses can index directly into per-space encoding tables with no
// string lookup at call time.
//
// The named helpers (writeTypeID, writeBuilderID, etc.) are non-virtual inline
// wrappers.  Each stores its IDSpaceID in a field initialised during
// construction so there is no lookup cost on the hot serialization path.
//

class IRSerializer : public Extensible {
    JBALLOC_(IRSerializer)

public:
    DYNAMIC_ALLOC_ONLY(IRSerializer, Extension *ext, KINDTYPE(Extensible) kind=KIND(Extensible));

    // -----------------------------------------------------------------------
    // Primitive write operations — must be implemented by every subclass
    // -----------------------------------------------------------------------

    virtual void writeBool   (bool            v) = 0;
    virtual void writeInt8   (int8_t          v) = 0;
    virtual void writeInt16  (int16_t         v) = 0;
    virtual void writeInt32  (int32_t         v) = 0;
    virtual void writeInt64  (int64_t         v) = 0;
    virtual void writeUInt8  (uint8_t         v) = 0;
    virtual void writeUInt16 (uint16_t        v) = 0;
    virtual void writeUInt32 (uint32_t        v) = 0;
    virtual void writeUInt64 (uint64_t        v) = 0;
    virtual void writeFloat  (float           v) = 0;
    virtual void writeDouble (double          v) = 0;
    virtual void writeString (const String  & s) = 0;

    // Raw byte sequence (e.g. the backing bytes of a Literal value)
    virtual void writeBytes  (const uint8_t *data, size_t len) = 0;

    // -----------------------------------------------------------------------
    // IR identifier writes
    //
    // writeID() is the single extensible virtual: subclasses override it to
    // control encoding for any file-local ID space.  The IDSpaceID is a
    // compact integer obtained from IDSpaceService — no string lookup at
    // call time.
    //
    // The named helpers below are non-virtual inline wrappers.  Their
    // IDSpaceID values are stored as fields initialised in the constructor
    // (via IDSpaceService::idSpaceFor), so the hot path is a simple field
    // load followed by the virtual call.
    //
    // Extensions that declare new ID spaces call writeID() directly with
    // their own IDSpaceID obtained from IDSpaceService.
    // -----------------------------------------------------------------------

    // Core extensible hook — one virtual covers every ID space
    virtual void writeID(IDSpaceID space, uint64_t id) = 0;

    // Named wrappers for core ID spaces — inline, non-virtual
    void writeExtID       (uint64_t     id) { writeID(_spExtID,       id); }
    void writeTypeID      (TypeID       id) { writeID(_spTypeID,      id); }
    void writeActionID    (ActionID     id) { writeID(_spActionID,    id); }
    void writeLiteralID   (LiteralID    id) { writeID(_spLiteralID,   id); }
    void writeSymbolID    (SymbolID     id) { writeID(_spSymbolID,    id); }
    void writeEntryPointID(EntryPointID id) { writeID(_spEntryPointID,id); }
    void writeContextID   (ContextID    id) { writeID(_spContextID,   id); }
    void writeScopeID     (ScopeID      id) { writeID(_spScopeID,     id); }
    void writeBuilderID   (BuilderID    id) { writeID(_spBuilderID,   id); }
    void writeValueID     (ValueID      id) { writeID(_spValueID,     id); }
    void writeOperationID (OperationID  id) { writeID(_spOperationID, id); }

    // -----------------------------------------------------------------------
    // Format-level structural helpers
    // -----------------------------------------------------------------------

    // List count: the uint32 element count that precedes every list(T)
    virtual void writeCount(uint32_t count) = 0;

    // KindRef: an ExtensibleKind recorded as (owning fExtID, kindName) so it
    // can be resolved via KindService after extensions are loaded
    virtual void writeKindRef(uint64_t fExtID, const String & kindName) = 0;

    // -----------------------------------------------------------------------
    // Structural section markers
    // Text subclasses emit human-readable section headers/footers.
    // Binary subclasses may emit tag bytes or omit them entirely.
    // -----------------------------------------------------------------------

    virtual void beginSection(const String & name) = 0;
    virtual void endSection  (const String & name) = 0;

    // RAII helper: calls beginSection on construction, endSection on destruction
    class Section {
    public:
        Section(IRSerializer & s, const String & name)
            : _s(s), _name(name) { _s.beginSection(_name); }
        ~Section()               { _s.endSection(_name); }
    private:
        IRSerializer & _s;
        String         _name;
    };

protected:
    // IDSpaceID fields initialised once in the constructor — one per core space.
    IDSpaceID _spExtID;
    IDSpaceID _spTypeID;
    IDSpaceID _spActionID;
    IDSpaceID _spLiteralID;
    IDSpaceID _spSymbolID;
    IDSpaceID _spEntryPointID;
    IDSpaceID _spContextID;
    IDSpaceID _spScopeID;
    IDSpaceID _spBuilderID;
    IDSpaceID _spValueID;
    IDSpaceID _spOperationID;

    SUBCLASS_KINDSERVICE_DECL(Extensible, IRSerializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(IRSERIALIZER_INCL)
