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

#ifndef CODEGENERATOR_INCL
#define CODEGENERATOR_INCL

#include <map>
#include "Transformer.hpp"

namespace TR { class IlBuilder; }
namespace TR { class IlType; }
namespace TR { class IlValue; }
namespace TR { class MethodBuilder; }
namespace TR { class TypeDictionary; }

namespace OMR
{

namespace JitBuilder
{

class Builder;
class FunctionBuilder;
class Operation;
class PointerType;
class Type;
class Value;

class CodeGenerator : public Transformer
   {
public:
   CodeGenerator(FunctionBuilder * fb, TR::MethodBuilder * mb);

   TR::MethodBuilder *mb()    { return _mb; }
   void * entryPoint() const  { return _entryPoint; }
   int32_t returnCode() const { return _compileReturnCode; }

   void generateFunctionAPI(FunctionBuilder *fb);

protected:
   virtual Builder * transformOperation(Operation *op);
   virtual FunctionBuilder * transformFunctionBuilderAtEnd(FunctionBuilder * fb);

   TR::IlBuilder *mapBuilder(Builder * b);
   void storeBuilder(Builder * b, TR::IlBuilder *omr_b);
   TR::IlType *mapPointerType(TR::TypeDictionary * types, PointerType * t);
   TR::IlType *mapType(Type * t);
   void storeType(Type * t, TR::IlType *omr_t);
   TR::IlValue *mapValue(Value * v);
   void storeValue(Value * v, TR::IlValue *omr_v);

   char * findOrCreateString(std::string str);

   void printAllMaps();

   std::map<uint64_t,TR::IlBuilder *> _builders;
   std::map<uint64_t,TR::IlType *> _types;
   std::map<uint64_t,TR::IlValue *> _values;
   std::map<uint64_t,TR::MethodBuilder *> _methodBuilders;
   std::map<uint64_t,TR::TypeDictionary *> _typeDictionaries;
   std::map<std::string,char *> _strings;

   TR::MethodBuilder *_mb;
   void *  _entryPoint;
   int32_t _compileReturnCode;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(CODEGENERATOR_INCL)
