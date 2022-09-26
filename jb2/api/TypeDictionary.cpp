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
#include <string>
#include "Compiler.hpp"
#include "Operation.hpp"
#include "TextWriter.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

TypeDictionary::TypeDictionary(Compiler *compiler)
    : _id(compiler->getTypeDictionaryID())
    , _compiler(compiler)
    , _name("")
    , _nextTypeID(0)
    , _linkedDictionary(NULL) {
}

TypeDictionary::TypeDictionary(Compiler *compiler, std::string name)
    : _id(compiler->getTypeDictionaryID())
    , _compiler(compiler)
    , _name(name)
    , _nextTypeID(0)
    , _linkedDictionary(NULL) {
}

// Only accessible to subclasses
TypeDictionary::TypeDictionary(Compiler *compiler, std::string name, TypeDictionary * linkedDict)
    : _id(compiler->getTypeDictionaryID())
    , _compiler(compiler)
    , _name(name)
    , _nextTypeID(linkedDict->_nextTypeID)
    , _linkedDictionary(linkedDict) {
    for (TypeIterator typeIt = linkedDict->TypesBegin(); typeIt != linkedDict->TypesEnd(); typeIt++) {
        const Type *type = *typeIt;
        internalRegisterType(type);
    }
    _nextTypeID = linkedDict->_nextTypeID;
}

TypeDictionary::~TypeDictionary() {
    for (auto it = _ownedTypes.begin(); it != _ownedTypes.end(); it++) {
        const Type *type = *it;
        delete type;
    }
}

const Type *
TypeDictionary::LookupType(TypeID id) {
    for (auto it = TypesBegin(); it != TypesEnd(); it++) {
        const Type *type = *it;
        if (type->id() == id)
            return type;
    }

    return NULL;
}

void
TypeDictionary::RemoveType(const Type *type) {
    // TODO should really collect these and do in one pass
    for (auto it = _types.begin(); it != _types.end(); ) {
        if (*it == type)
            it = _types.erase(it);
        else
            ++it;
   }
}

void
TypeDictionary::write(TextWriter &w) {
    w.indent() << "[ TypeDictionary " << this << " \"" << this->name() << "\"" << w.endl();
    w.indentIn();
    if (this->hasLinkedDictionary())
        w.indent() << "[ linkedDictionary " << this->linkedDictionary() << " ]" << w.endl();
    for (TypeIterator typeIt = this->TypesBegin();typeIt != this->TypesEnd();typeIt++) {
        const Type *type = *typeIt;
        w.indent();
        type->writeType(w, true);
        w << w.endl();
    }
    w.indentOut();
    w.indent() << "]" << w.endl();
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

