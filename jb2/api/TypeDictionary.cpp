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

#include <stdio.h>
#include "AllocationCategoryClasses.hpp"
#include "Compiler.hpp"
#include "Operation.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(TypeDictionary, Dictionaries)

TypeDictionary::TypeDictionary(Allocator *a, Compiler *compiler)
    : Allocatable(a)
    , _id(compiler->getTypeDictionaryID())
    , _name("")
    , _compiler(compiler)
    , _mem(compiler->mem())
    , _types(NULL, _mem)
    , _ownedTypes(NULL, _mem)
    , _nextTypeID(0)
    , _linkedDictionary(NULL) {
}

TypeDictionary::TypeDictionary(Compiler *compiler)
    : Allocatable()
    , _id(compiler->getTypeDictionaryID())
    , _name("")
    , _compiler(compiler)
    , _mem(compiler->mem())
    , _types(NULL, _mem)
    , _ownedTypes(NULL, _mem)
    , _nextTypeID(0)
    , _linkedDictionary(NULL) {
}

TypeDictionary::TypeDictionary(Allocator *a, Compiler *compiler, String name)
    : Allocatable(a)
    , _id(compiler->getTypeDictionaryID())
    , _name(name)
    , _compiler(compiler)
    , _mem(compiler->mem())
    , _types(NULL, _mem)
    , _ownedTypes(NULL, _mem)
    , _nextTypeID(0)
    , _linkedDictionary(NULL) {
}

TypeDictionary::TypeDictionary(Compiler *compiler, String name)
    : _id(compiler->getTypeDictionaryID())
    , _name(name)
    , _compiler(compiler)
    , _mem(compiler->mem())
    , _types(NULL, _mem)
    , _ownedTypes(NULL, _mem)
    , _nextTypeID(0)
    , _linkedDictionary(NULL) {
}

// Only accessible to subclasses
TypeDictionary::TypeDictionary(Allocator *a, Compiler *compiler, String name, TypeDictionary * linkedDict)
    : Allocatable(a)
    , _id(compiler->getTypeDictionaryID())
    , _name(name)
    , _compiler(compiler)
    , _mem(compiler->mem())
    , _types(NULL, _mem)
    , _ownedTypes(NULL, _mem)
    , _nextTypeID(linkedDict->_nextTypeID)
    , _linkedDictionary(linkedDict) {
    for (auto it = linkedDict->typesIterator(); it.hasItem(); it++) {
        const Type *type = it.item();
        internalRegisterType(type);
    }
    _nextTypeID = linkedDict->_nextTypeID;
}

// Only accessible to subclasses
TypeDictionary::TypeDictionary(Compiler *compiler, String name, TypeDictionary * linkedDict)
    : Allocatable()
    , _id(compiler->getTypeDictionaryID())
    , _name(name)
    , _compiler(compiler)
    , _mem(compiler->mem())
    , _types(NULL, _mem)
    , _ownedTypes(NULL, _mem)
    , _nextTypeID(linkedDict->_nextTypeID)
    , _linkedDictionary(linkedDict) {
    for (auto it = linkedDict->typesIterator(); it.hasItem(); it++) {
        const Type *type = it.item();
        internalRegisterType(type);
    }
    _nextTypeID = linkedDict->_nextTypeID;
}

TypeDictionary::~TypeDictionary() {
    for (auto it = _ownedTypes.iterator(true, false, false); it.hasItem(); it++) {
        const Type *type = it.item();
        delete type;
    }
    _ownedTypes.erase();
}

const Type *
TypeDictionary::LookupType(TypeID id) {
    for (auto it = typesIterator(); it.hasItem(); it++) {
        const Type *type = it.item();
        if (type->id() == id)
            return type;
    }

    return NULL;
}

void
TypeDictionary::RemoveType(const Type *type) {
    // TODO should really collect these and do in one pass
    auto it = _types.find(type);
    if (it.hasItem())
        _types.remove(it);
}

void
TypeDictionary::log(TextLogger &lgr) {
    lgr.indent() << "[ TypeDictionary " << this << " \"" << this->name() << "\"" << lgr.endl();
    lgr.indentIn();
    if (this->hasLinkedDictionary())
        lgr.indent() << "[ linkedDictionary " << this->linkedDictionary() << " ]" << lgr.endl();
    for (auto it = this->typesIterator();it.hasItem();it++) {
        const Type *type = it.item();
        lgr.indent();
        type->logType(lgr, true);
        lgr << lgr.endl();
    }
    lgr.indentOut();
    lgr.indent() << "]" << lgr.endl();
}

void
TypeDictionary::internalRegisterType(const Type *type) {
    _types.push_back(type);
}

void
TypeDictionary::registerType(const Type *type) {
    internalRegisterType(type);
    _ownedTypes.push_back(type);
}

#if 0
// keep this handy in case needed during migration
void
TypeDictionary::RemoveType(Type *type)
   {

   if (type->isField())
      ; // nothing special for fields themselves
   else if (type->isStruct() || type->isUnion())
      {
      StructType *sType = static_cast<StructType *>(type);
      bool fullyRemove = (type->owningDictionary() == this);
      for (auto it = sType->FieldsBegin(); it != sType->FieldsEnd(); )
         {
         FieldType *fType = it->second;
         forgetType(fType);

         if (fullyRemove)
            it = sType->RemoveField(it);
         else
            it++;
         }

      _structTypeFromName.erase(type->name());
      }
   else if (type->isPointer())
      {
      PointerType *ptrType = static_cast<PointerType *>(type);
      _pointerTypeFromBaseType.erase(ptrType->BaseType());
      }
   else if (type->isFunction())
      _functionTypeFromName.erase(type->name());

   forgetType(type);

   // TODO but not *strictly* necessary: _graph->unregister(type);
   }

#endif

} // namespace JitBuilder
} // namespace OMR

