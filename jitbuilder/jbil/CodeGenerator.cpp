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

#include "CodeGenerator.hpp"
#include "Builder.hpp"
#include "FunctionBuilder.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "PrettyPrinter.hpp"
#include "Type.hpp"
#include "Value.hpp"

#include "ilgen/IlBuilder.hpp"
#include "ilgen/IlType.hpp"
#include "ilgen/IlValue.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "env/FrontEnd.hpp"
#include "env/TRMemory.hpp"

using namespace OMR::JitBuilder;

CodeGenerator::CodeGenerator(FunctionBuilder * fb, TR::MethodBuilder * mb)
   : Transformer(fb)
   , _mb(mb)
   {
   setTraceEnabled(fb->config()->traceCodeGenerator());
   }

TR::IlBuilder *
CodeGenerator::mapBuilder(Builder * b)
   {
   if (_builders.find(b->id()) == _builders.end())
      storeBuilder(b, mapBuilder(b->parent())->OrphanBuilder());
   return _builders[b->id()];
   }

void
CodeGenerator::storeBuilder(Builder * b, TR::IlBuilder *omr_b)
   {
   _builders[b->id()] = omr_b;
   }

TR::IlType *
CodeGenerator::mapType(Type * t)
   {
   return _types[t->id()];
   }

TR::IlType *
CodeGenerator::mapPointerType(TR::TypeDictionary * types, PointerType * t)
   {
   if (_types.find(t->id()) != _types.end())
      return mapType(t);

   Type * baseType = t->BaseType();
   TR::IlType *baseIlType;
   if (baseType->isPointer())
      baseIlType = mapPointerType(types, (PointerType *)baseType);
   else
      baseIlType = mapType(baseType);

   TR::IlType *ptrIlType;
   if (baseIlType)
      ptrIlType = types->PointerTo(baseIlType);
   else
      {
      // must be user type 
      // hacky, cook up a struct type of baseType's name (and doesn't get size!)
      char *typeName = findOrCreateString(baseType->name());
      ptrIlType = types->DefineStruct(typeName);
      types->CloseStruct(typeName);

      // store so we don't do this anymore
      storeType(baseType, ptrIlType);
      }

   return ptrIlType;
   }

void
CodeGenerator::storeType(Type * t, TR::IlType *omr_t)
   {
   _types[t->id()] = omr_t;
   }

TR::IlValue *
CodeGenerator::mapValue(Value * v)
   {
   return _values[v->id()];
   }

void
CodeGenerator::storeValue(Value * v, TR::IlValue *omr_v)
   {
   _values[v->id()] = omr_v;
   }

char *
CodeGenerator::findOrCreateString(std::string str)
   {
   if (_strings.find(str) != _strings.end())
      return _strings[str];

   char *s = new char[str.length()+1];
   strcpy(s, str.c_str());
   _strings[str] = s;
   return s;
   }

void
OMR::JitBuilder::CodeGenerator::printAllMaps()
   {
   PrettyPrinter * pLog = _fb->logger();
   if (traceEnabled() && pLog)
      {
      PrettyPrinter & log = *pLog;

      log << "[ printAllMaps" << log.endl();
      log.indentIn();

      log.indent() << "[ Builders" << log.endl();
      log.indentIn();
      for (auto builderIt = _builders.cbegin(); builderIt != _builders.cend(); builderIt++)
         {
         log.indent() << "[ builder " << builderIt->first << " -> TR::IlBuilder " << (int64_t *)(void *) builderIt->second << " ]" << log.endl();
         }
      log.indentOut();
      log.indent() << "]" << log.endl();

      log.indent() << "[ Values" << log.endl();
      log.indentIn();
      for (auto valueIt = _values.cbegin(); valueIt != _values.cend(); valueIt++)
         {
         log.indent() << "[ value " << valueIt->first << " -> TR::IlValue " << (int64_t *)(void *) valueIt->second << " ]" << log.endl();
         }
      log.indentOut();
      log.indent() << "]" << log.endl();

      log.indent() << "[ Types" << log.endl();
      log.indentIn();
      for (auto typeIt = _types.cbegin(); typeIt != _types.cend(); typeIt++)
         {
         log.indent() << "[ type " << typeIt->first << " -> TR::IlType " << (int64_t *)(void *) typeIt->second << " ]" << log.endl();
         }
      log.indentOut();
      log.indent() << "]" << log.endl();

      log.indentOut();
      log.indent() << "]" << log.endl();
      }
   }

void
OMR::JitBuilder::CodeGenerator::generateFunctionAPI(FunctionBuilder *fb)
   {
   TR::TypeDictionary * types = _mb->typeDictionary();
   _typeDictionaries[fb->types()->id()] = types;

   // TODO: iterate Types to create TR::IlType mappings
   storeType(fb->types()->NoType,  types->NoType );
   storeType(fb->types()->Int8,    types->Int8   );
   storeType(fb->types()->Int16,   types->Int16  );
   storeType(fb->types()->Int32,   types->Int32  );
   storeType(fb->types()->Int64,   types->Int64  );
   storeType(fb->types()->Float,   types->Float  );
   storeType(fb->types()->Double,  types->Double );
   storeType(fb->types()->Address, types->Address);
   storeType(fb->types()->Word,    types->Word);
   for (TypeIterator typeIt = fb->types()->TypesBegin(); typeIt != fb->types()->TypesEnd(); typeIt++)
      {
      Type * type = *typeIt;
      if (type->isPointer())
         {
         TR::IlType *ptrIlType = mapPointerType(types, (PointerType *)type);
         storeType(type, ptrIlType);
         }
      else
         ; // primitive types already handled; eventually handle other types too
      }

   _methodBuilders[fb->id()] = _mb;
   storeBuilder(fb, _mb);

   _mb->DefineName(findOrCreateString(fb->name()));
   _mb->DefineFile(findOrCreateString(fb->fileName()));
   _mb->DefineLine(findOrCreateString(fb->lineNumber()));
   _mb->DefineReturnType(mapType(fb->getReturnType()));

   for (ParameterSymbolIterator paramIt = fb->ParametersBegin();paramIt != fb->ParametersEnd(); paramIt++)
      {
      const ParameterSymbol &parameter = *paramIt;
      char *paramName = findOrCreateString(parameter.name());
      _mb->DefineParameter(paramName, mapType(parameter.type()));
      }
   for (SymbolIterator localIt = fb->LocalsBegin();localIt != fb->LocalsEnd();localIt++)
      {
      const Symbol &local = *localIt;
      char *localName = findOrCreateString(local.name());
      _mb->DefineLocal(localName, mapType(local.type()));
      }
   }

Builder *
CodeGenerator::transformOperation(Operation * op)
   {
   Builder * b = op->parent();
   TR::IlBuilder *omr_b = mapBuilder(b);
   omr_b->setBCIndex(op->location()->bcIndex())->SetCurrentIlGenerator();
   switch (op->action())
      {
      case aConstInt8 :
         storeValue(op->result(), omr_b->ConstInt8(op->getLiteralByte()));
         break;

      case aConstInt16 :
         storeValue(op->result(), omr_b->ConstInt16(op->getLiteralShort()));
         break;

      case aConstInt32 :
         storeValue(op->result(), omr_b->ConstInt32(op->getLiteralInteger()));
         break;

      case aConstInt64 :
         storeValue(op->result(), omr_b->ConstInt64(op->getLiteralLong()));
         break;

      case aConstFloat :
         storeValue(op->result(), omr_b->ConstFloat(op->getLiteralFloat()));
         break;

      case aConstDouble :
         storeValue(op->result(), omr_b->ConstDouble(op->getLiteralDouble()));
         break;

      case aCoercePointer :
         storeValue(op->result(), mapValue(op->operand()));
         break;

      case aAdd :
         storeValue(op->result(), omr_b->Add(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aSub :
         storeValue(op->result(), omr_b->Sub(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aMul :
         storeValue(op->result(), omr_b->Mul(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aLoad :
         storeValue(op->result(), omr_b->Load(findOrCreateString(op->getLiteralString())));
         break;

      case aLoadAt :
         storeValue(op->result(), omr_b->LoadAt(mapType(op->type()), mapValue(op->operand())));
         break;

      case aStore :
         omr_b->Store(findOrCreateString(op->getLiteralString()), mapValue(op->operand()));
         break;

      case aStoreAt :
         omr_b->StoreAt(mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIndexAt :
         storeValue(op->result(), omr_b->IndexAt(mapType(op->type()), mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aAppendBuilder :
         omr_b->AppendBuilder(mapBuilder(op->builder()));
         break;

      case aReturn :
         if (op->numOperands() > 0)
            omr_b->Return(mapValue(op->operand()));
         else
            omr_b->Return();
         break;

      case aIfCmpGreaterThan :
         omr_b->IfCmpGreaterThan(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpLessThan :
         omr_b->IfCmpLessThan(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpGreaterOrEqual :
         omr_b->IfCmpGreaterOrEqual(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpLessOrEqual :
         omr_b->IfCmpLessOrEqual(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfThenElse :
      case aForLoop :
      case aSwitch :
         assert(0); // not (yet?) supported
         break;

      default :
         assert(0); // unknown action!!
         break;
      }

   return NULL;
   }

FunctionBuilder *
OMR::JitBuilder::CodeGenerator::transformFunctionBuilderAtEnd(FunctionBuilder * fb)
   { 
   printAllMaps();
   return NULL;
   }
