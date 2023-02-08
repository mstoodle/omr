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

#ifndef TYPEDICTIONARY_INCL
#define TYPEDICTIONARY_INCL

#include "common.hpp"
#include "String.hpp"

namespace OMR {
namespace JitBuilder {

class Compiler;
class DebugDictionary;
class DynamicType;
class Extension;
class OperationBuilder;

class TypeDictionary : public Allocatable {
    JBALLOC_(TypeDictionary)

    friend class DynamicType;
    friend class Extension;
    friend class OperationBuilder;
    friend class Type;

public:
    ALL_ALLOC_ALLOWED(TypeDictionary, Compiler *compiler);
    ALL_ALLOC_ALLOWED(TypeDictionary, Compiler *compiler, String name);
    ALL_ALLOC_ALLOWED(TypeDictionary, Compiler *compiler, String name, TypeDictionary *linkedDict);

    Compiler *compiler() const { return _compiler; }

    //TypeIterator TypesBegin() const { return TypeIterator(_types); }
    //TypeIterator TypesEnd() const { return TypeIterator(); }
    TypeListIterator typesIterator() const { return _types.iterator(); }
    TypeListIterator modifiableTypesIterator() { return _types.iterator(true, false, false); }

    const Type *LookupType(uint64_t id);
    void RemoveType(const Type *type);
    TypeID numTypes() const { return _nextTypeID; }

    TypeDictionaryID id() const { return _id; }
    String name() const { return _name; }
    bool hasLinkedDictionary() const { return _linkedDictionary != NULL; }
    TypeDictionary *linkedDictionary() { return _linkedDictionary; }

    void log(TextLogger &lgr);

    void registerType(const Type *type);

protected:
    void internalRegisterType(const Type *type);
    TypeID getTypeID() { return _nextTypeID++; }

    TypeDictionaryID _id;
    String _name;
    Compiler * _compiler;
    Allocator * _mem;
    List<const Type *> _types;
    List<const Type *> _ownedTypes;
    TypeID _nextTypeID;
    TypeDictionary * _linkedDictionary;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(TYPEDICTIONARY_INCL)

