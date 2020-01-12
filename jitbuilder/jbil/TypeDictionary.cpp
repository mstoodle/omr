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

#include <string>
#include "Operation.hpp"
#include "Type.hpp"
#include "TypeGraph.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

using namespace OMR::JitBuilder;

int64_t OMR::JitBuilder::TypeDictionary::globalIndex = 0;

Type * OMR::JitBuilder::TypeDictionary::NoType  = Type::NoType;
Type * OMR::JitBuilder::TypeDictionary::Int8    = TypeDictionary::createType("Int8");
Type * OMR::JitBuilder::TypeDictionary::Int16   = TypeDictionary::createType("Int16");
Type * OMR::JitBuilder::TypeDictionary::Int32   = TypeDictionary::createType("Int32");
Type * OMR::JitBuilder::TypeDictionary::Int64   = TypeDictionary::createType("Int64");
Type * OMR::JitBuilder::TypeDictionary::Float   = TypeDictionary::createType("Float");
Type * OMR::JitBuilder::TypeDictionary::Double  = TypeDictionary::createType("Double");
Type * OMR::JitBuilder::TypeDictionary::Address = TypeDictionary::createType("Address");

// Should really be either Int64 or Int32 based on Config
Type * OMR::JitBuilder::TypeDictionary::Word = OMR::JitBuilder::TypeDictionary::Int64;

TypeDictionary::TypeDictionary()
   : _id(globalIndex++)
   , _name("")
   , _graph(new TypeGraph(this))
   {
   initializeGraph();
   }

TypeDictionary::TypeDictionary(std::string name)
   : _id(globalIndex++)
   , _name(name)
   , _graph(new TypeGraph(this))
   {
   initializeGraph();
   }

TypeDictionary::TypeDictionary(TypeGraph * graph)
   : _id(globalIndex++)
   , _name("")
   , _graph(graph)
   {
   initializeGraph();
   }

TypeDictionary::TypeDictionary(std::string name, TypeGraph * graph)
   : _id(globalIndex++)
   , _name("")
   , _graph(graph)
   {
   initializeGraph();
   }

void
TypeDictionary::initializeGraph()
   {
   _types.push_back(NoType);
   _types.push_back(Int8);
   _types.push_back(Int16);
   _types.push_back(Int32);
   _types.push_back(Int64);
   _types.push_back(Float);
   _types.push_back(Double);
   _types.push_back(Address);

   _graph->registerType(NoType);
   _graph->registerType(Int8);
   _graph->registerType(Int16);
   _graph->registerType(Int32);
   _graph->registerType(Int64);
   _graph->registerType(Float);
   _graph->registerType(Double);
   _graph->registerType(Address);

   Add::initializeTypeProductions(this, _graph);
   Sub::initializeTypeProductions(this, _graph);
   Mul::initializeTypeProductions(this, _graph);

   ForLoop::initializeTypeProductions(this, _graph);
   IfCmpGreaterOrEqual::initializeTypeProductions(this, _graph);
   IfCmpGreaterThan::initializeTypeProductions(this, _graph);
   IfCmpLessOrEqual::initializeTypeProductions(this, _graph);
   IfCmpLessThan::initializeTypeProductions(this, _graph);
   IfThenElse::initializeTypeProductions(this, _graph);
   Return::initializeTypeProductions(this, _graph);
   Switch::initializeTypeProductions(this, _graph);
   }

void
OMR::JitBuilder::TypeDictionary::registerPointerType(PointerType * pointerType)
   {
   _graph->registerType(pointerType);

   Type * baseType = pointerType->BaseType();
   _graph->registerValidOperation(pointerType, aAdd, pointerType, Word);
   _graph->registerValidOperation(Word, aSub, pointerType, pointerType);

   _graph->registerValidOperation(pointerType, aIndexAt, pointerType, Word);
   if (Word != Int32)
      _graph->registerValidOperation(pointerType, aIndexAt, pointerType, Int32);

   _graph->registerValidOperation(baseType, aLoadAt, pointerType);
   _graph->registerValidOperation(NoType, aStoreAt, pointerType, baseType);
   }

Type *
OMR::JitBuilder::TypeDictionary::producedType(Action a, Value *v)
   {
   return _graph->producedType(a, v->type());
   }

Type *
OMR::JitBuilder::TypeDictionary::producedType(Action a, Value *left, Value *right)
   {
   return _graph->producedType(a, left->type(), right->type());
   }

Type *
OMR::JitBuilder::TypeDictionary::producedType(Action a, Value *one, Value *two, Value *three)
   {
   return _graph->producedType(a, one->type(), two->type(), three->type());
   }

PointerType *
TypeDictionary::PointerTo(Type * baseType)
   {
   // don't replicate types
   std::map<Type *,PointerType *>::iterator found = _pointerTypeFromBaseType.find(baseType);
   if (found != _pointerTypeFromBaseType.end())
      {
      PointerType *t = found->second;
      return t;
      }

   PointerType * newType = PointerType::create(baseType, std::string("PointerTo(") + baseType->name()  + std::string(")"));
   _types.push_back(newType);
   _pointerTypeFromBaseType[baseType] = newType;
   registerPointerType(newType);
   return newType;
   }

Type *
TypeDictionary::DefineStruct(std::string structName)
   {
   // don't replicate types
   std::map<std::string,Type *>::iterator found = _structTypeFromName.find(structName);
   if (found != _structTypeFromName.end())
      {
      Type *t = found->second;
      return t;
      }

   Type * newType = Type::create(structName);
   _types.push_back(newType);
   _graph->registerType(newType);
   return newType;
   }

void
TypeDictionary::DefineField(std::string structName, std::string fieldName, Type * fieldType)
   {
   }

void
TypeDictionary::CloseStruct(std::string structName)
   {
   }
