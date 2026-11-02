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

#ifndef IRDESERIALIZER_INCL
#define IRDESERIALIZER_INCL

#include <cstddef>
#include <cstdint>
#include <utility>
#include "Array.hpp"
#include "Extensible.hpp"
#include "IDSpaceService.hpp"
#include "IDs.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Extension;

//
// IRDeserializer is the abstract base class for all IR deserialization strategies.
// It is a pure stream-reading primitive — it has no knowledge of IR elements.
//
// IRDeserializer mirrors IRSerializer exactly: every write*() method on
// IRSerializer has a corresponding read*() method here.  It only understands
// scalars, raw bytes, typed identifiers, and structural section markers.
//
// Reconstruction of IR elements is driven by IR::read(IRDeserializer &), which
// creates an IRReader (an IRCloner subclass) to coordinate element construction.
// IRReader passes the IRDeserializer to each element's clone-path constructor
// so that the existing IRCloner machinery — ID-indexed arrays, cross-reference
// resolution — is reused without duplication.
//
// Class hierarchy:
//
//   IRDeserializer                      (this file — abstract)
//     ├── TextDeserializer              (TextDeserializer.hpp — parses human-readable text)
//     └── BinaryDeserializer            (BinaryDeserializer.hpp — binary mid-layer, abstract)
//           └── BasicBinaryDeserializer (BasicBinaryDeserializer.hpp — raw fixed-width decoding)
//
// readID() takes an IDSpaceID — a compact integer registered with
// IDSpaceService — mirroring IRSerializer::writeID().  IDSpaceID values for
// the core spaces are stored as fields initialised during construction.
//

class IRDeserializer : public Extensible {
    JBALLOC_(IRDeserializer)

public:
    DYNAMIC_ALLOC_ONLY(IRDeserializer, Extension *ext, KINDTYPE(Extensible) kind=KIND(Extensible));

    // -----------------------------------------------------------------------
    // Primitive read operations — must be implemented by every subclass
    // -----------------------------------------------------------------------

    virtual void readBool   (bool     & v) = 0;
    virtual void readInt8   (int8_t   & v) = 0;
    virtual void readInt16  (int16_t  & v) = 0;
    virtual void readInt32  (int32_t  & v) = 0;
    virtual void readInt64  (int64_t  & v) = 0;
    virtual void readUInt8  (uint8_t  & v) = 0;
    virtual void readUInt16 (uint16_t & v) = 0;
    virtual void readUInt32 (uint32_t & v) = 0;
    virtual void readUInt64 (uint64_t & v) = 0;
    virtual void readFloat  (float    & v) = 0;
    virtual void readDouble (double   & v) = 0;
    virtual void readString (String   & s) = 0;

    // Raw byte sequence — caller must provide a buffer of at least len bytes
    virtual void readBytes  (uint8_t *data, size_t len) = 0;

    // -----------------------------------------------------------------------
    // IR identifier reads
    //
    // readID() is the single extensible virtual: subclasses override it to
    // control decoding for any file-local ID space.  The IDSpaceID is a
    // compact integer obtained from IDSpaceService — mirrors writeID().
    //
    // The named helpers below are non-virtual inline wrappers.  Their
    // IDSpaceID values are stored as fields initialised in the constructor
    // (via IDSpaceService::idSpaceFor), so the hot path is a simple field
    // load followed by the virtual call.
    //
    // Extensions that declare new ID spaces call readID() directly with
    // their own IDSpaceID obtained from IDSpaceService.
    // -----------------------------------------------------------------------

    // Core extensible hook — one virtual covers every ID space
    virtual uint64_t readID(IDSpaceID space) = 0;

    // Named wrappers for core ID spaces — inline, non-virtual
    uint64_t     readExtID()        { return                                    readID(_spExtID);        }
    TypeID       readTypeID()       { return static_cast<TypeID>       (readID(_spTypeID));       }
    ActionID     readActionID()     { return static_cast<ActionID>     (readID(_spActionID));     }
    LiteralID    readLiteralID()    { return static_cast<LiteralID>    (readID(_spLiteralID));    }
    SymbolID     readSymbolID()     { return static_cast<SymbolID>     (readID(_spSymbolID));     }
    EntryPointID readEntryPointID() { return static_cast<EntryPointID> (readID(_spEntryPointID)); }
    ContextID    readContextID()    { return static_cast<ContextID>    (readID(_spContextID));    }
    ScopeID      readScopeID()      { return static_cast<ScopeID>      (readID(_spScopeID));      }
    BuilderID    readBuilderID()    { return static_cast<BuilderID>    (readID(_spBuilderID));    }
    ValueID      readValueID()      { return static_cast<ValueID>      (readID(_spValueID));      }
    OperationID  readOperationID()  { return static_cast<OperationID>  (readID(_spOperationID));  }

    // -----------------------------------------------------------------------
    // File-local ID to runtime object mapping tables
    // -----------------------------------------------------------------------

    void mapExtension(uint64_t fExtID, Extension *ext) {
        _fExtToExtension.assign(fExtID, ext);
    }
    Extension *extension(uint64_t fExtID) const {
        assert(fExtID > 0 && fExtID < _fExtToExtension.length());
        return _fExtToExtension[fExtID];
    }
    const Array<Extension *> & extensions() const { return _fExtToExtension; }

    void mapAction(ActionID fActionID, ActionID liveActionID) {
        _fActionToAction.assign(fActionID, liveActionID);
    }
    ActionID action(ActionID fActionID) const {
        assert(fActionID > 0 && fActionID < _fActionToAction.length());
        return _fActionToAction[fActionID];
    }
    const Array<ActionID> & actions() const { return _fActionToAction; }

    // -----------------------------------------------------------------------
    // Format-level structural helpers
    // -----------------------------------------------------------------------

    // List count: read the uint32 element count preceding a list(T)
    virtual uint32_t readCount() = 0;

    // KindRef: read the (fExtID, kindName) pair written by writeKindRef.
    // Callers receive both components as return-by-value equivalents:
    //   auto [ext, name] = s.readKindRef();   (C++17)
    // or assign to a std::pair explicitly.
    virtual std::pair<uint64_t, String> readKindRef() = 0;

    // -----------------------------------------------------------------------
    // Structural section markers
    // Text subclasses consume and validate section headers/footers.
    // Binary subclasses consume tag bytes, or do nothing.
    // -----------------------------------------------------------------------

    virtual void beginSection(const String & name) = 0;
    virtual void endSection  (const String & name) = 0;

    // RAII helper: calls beginSection on construction, endSection on destruction
    class Section {
    public:
        Section(IRDeserializer & s, const String & name)
            : _s(s), _name(name) { _s.beginSection(_name); }
        ~Section()               { _s.endSection(_name); }
    private:
        IRDeserializer & _s;
        String           _name;
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

    Array<Extension *> _fExtToExtension;
    Array<ActionID>    _fActionToAction;

    SUBCLASS_KINDSERVICE_DECL(Extensible, IRDeserializer);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(IRDESERIALIZER_INCL)
