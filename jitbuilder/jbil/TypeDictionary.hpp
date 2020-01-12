/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
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


#include <stdint.h>
#include <map>
#include <vector>
#include "Action.hpp"
#include "Iterator.hpp"
#include "Operation.hpp"
#include "Type.hpp"

namespace OMR
{

namespace JitBuilder
{

class TypeGraph;

class TypeDictionary
   {
public:
   TypeDictionary();
   TypeDictionary(std::string name);
   TypeDictionary(TypeGraph * graph);
   TypeDictionary(std::string name, TypeGraph * graph);

   static Type * NoType;
   static Type * Int8;
   static Type * Int16;
   static Type * Int32;
   static Type * Int64;
   static Type * Float;
   static Type * Double;
   static Type * Address;
   static Type * Word;

   PointerType * PointerTo(Type * baseType);
   Type * DefineStruct(std::string structName);
   void DefineField(std::string structName, std::string fieldName, Type * fieldType);
   void CloseStruct(std::string structName);

   TypeIterator TypesBegin() const { return TypeIterator(_types); }
   TypeIterator TypesEnd() const   { return TypeIterator(); }

   int64_t id() const       { return _id; }
   std::string name() const { return _name; }

   // get the Type produced by Action a on Value v
   Type * producedType(Action a, Value *v);

   // get the Type produced by Action a on Values left and right
   Type * producedType(Action a, Value *left, Value *right);

   // get the Type produced by Action a on Values one, two, and three
   Type * producedType(Action a, Value *one, Value *two, Value *three);

protected:
   static Type * createType(std::string name) { return Type::create(name); }

   void initializeGraph();
   void registerPointerType(PointerType * pointerType);

   int64_t                       _id;
   std::string                   _name;
   std::vector<Type *>           _types;
   std::map<Type *,PointerType*> _pointerTypeFromBaseType;
   std::map<std::string,Type *>  _structTypeFromName;
   TypeGraph                   * _graph;

   static int64_t globalIndex;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(TYPEDICTIONARY_INCL)
