/*******************************************************************************
 * Copyright (c) 2018, 2018 IBM Corp. and others
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

#include "ilgen/VectorLoopBuilder.hpp"

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "codegen/CodeGenerator.hpp"
#include "compile/Compilation.hpp"
#include "compile/Method.hpp"
#include "compile/SymbolReferenceTable.hpp"
#include "control/Recompilation.hpp"
#include "env/CompilerEnv.hpp"
#include "env/FrontEnd.hpp"
#include "il/Block.hpp"
#include "il/SymbolReference.hpp"
#include "il/ILOps.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/TreeTop.hpp"
#include "il/TreeTop_inlines.hpp"
#include "il/symbol/AutomaticSymbol.hpp"
#include "ilgen/IlGeneratorMethodDetails_inlines.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "ilgen/IlInjector.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/BytecodeBuilder.hpp"
#include "infra/Cfg.hpp"
#include "infra/List.hpp"

#define OPT_DETAILS "O^O ILBLD: "

#define TraceEnabled    (comp()->getOption(TR_TraceILGen))
#define TraceIL(m, ...) {if (TraceEnabled) {traceMsg(comp(), m, ##__VA_ARGS__);}}


OMR::VectorLoopBuilder::VectorLoopBuilder(TR::Compilation *comp,
                                          TR::MethodBuilder *methodBuilder,
                                          TR::TypeDictionary *types,
                                          TR::IlType *vectorElementType)
   : TR::IlBuilder(methodBuilder, types),
   _parent(NULL),
   _vectorElementType(vectorElementType),
   _vectorValueMap(),
   _vectorNameMap(),
   _vectorLoopBody(methodBuilder, types),
   _vectorIteratorName(0),
   _residueLoopBody(methodBuilder, types),
   _residueIteratorName(0)
   {
   static uint32_t vectorLoopNumber = 1; // /leave 0 as a sentinel value

   _loopID = vectorLoopNumber++;
   _vectorIteratorName = (char *) comp->trMemory()->allocateHeapMemory(6+10+0); // 6 (_viter_) + 10 (maxint) + trailing zero
   sprintf(_vectorIteratorName, "_viter_%u", _loopID);

   _residueIteratorName = (char *) comp->trMemory()->allocateHeapMemory(8+10+0); // 8 (_resiter_) + 10 (maxint) + trailing zero
   sprintf(_residueIteratorName, "_resiter_%u", _loopID);

   _residueConditionName = (char *) comp->trMemory()->allocateHeapMemory(11+10+0); // 11 (_rescontinue_) + 10 (maxint) + trailing zero
   sprintf(_residueConditionName, "_rescontinue_%u", _loopID);

   // won't be initialized otherwise
   _comp = comp;

   _hasVectorLoop = (VectorLength() > 1);
   }

void
OMR::VectorLoopBuilder::initialize(TR::IlGeneratorMethodDetails * details,
                                   TR::ResolvedMethodSymbol     * methodSymbol,
                                   TR::FrontEnd                 * fe,
                                   TR::SymbolReferenceTable     * symRefTab)
   {
   OMR::IlInjector::initialize(details, methodSymbol, fe, symRefTab);
   _residueLoopBody.initialize(details, methodSymbol, fe, symRefTab);
   if (_hasVectorLoop)
      _vectorLoopBody.initialize(details, methodSymbol, fe, symRefTab);
   }

void
OMR::VectorLoopBuilder::setupForBuildIL()
   {
   _residueLoopBody.setupForBuildIL();
   if (_hasVectorLoop)
      _vectorLoopBody.setupForBuildIL();
   }

TR::IlValue *
OMR::VectorLoopBuilder::getVectorValue(TR::IlValue *value)
   {
   if (!_hasVectorLoop)
      return NULL;

   // if this value was created inside this vector loop builder, it will have a mapping here
   if (_vectorValueMap.find(value) != _vectorValueMap.end())
      return _vectorValueMap[value];

   // otherwise, if there's a parent vector loop builder check if it's a value from there
   TR::VectorLoopBuilder *parent = parentVectorLoopBuilder();
   if (parent)
      return parent->getVectorValue(value);

   // otherwise, it's a value that comes from outside the vector loop, so we can just use it in both loops
   return value;
   }

char *
OMR::VectorLoopBuilder::getVectorName(const char *name)
   {
   if (!_hasVectorLoop)
      return NULL;

   // if this value was created inside this vector loop builder, it will have a mapping here
   if (_vectorNameMap.find(name) != _vectorNameMap.end())
      return _vectorNameMap[name];

   // otherwise, if there's a parent vector loop builder check if it's a value from there
   TR::VectorLoopBuilder *parent = parentVectorLoopBuilder();
   if (parent)
      return parent->getVectorName(name);

   // otherwise, allocate the name here

#define MAX_SUPPORTED_NAME_LEN 128

   size_t nameLength = MAX_SUPPORTED_NAME_LEN;
   const char *term = (const char *) memchr(name,'\0',MAX_SUPPORTED_NAME_LEN);
   if (term != NULL)
      nameLength = term - name;

   size_t maxLength = 8+nameLength+1; // 8 (_vector_) + strlen(name) + trailing zero
   char *vectorName = (char *) comp()->trMemory()->allocateHeapMemory(maxLength);
   memcpy(vectorName, "_vector_", 8);
   memcpy(vectorName+8, name, nameLength);
   vectorName[8+nameLength] = '\0';
   _vectorNameMap[name] = vectorName;

   return vectorName;
   }

uint32_t
OMR::VectorLoopBuilder::VectorLength()
   {
   // currently assumes vectors are 128 bit registers
   // should really be a query into the OMR compiler's code generator object

   if (_vectorElementType == Double || _vectorElementType == Int64)
      return 2;
   if (_vectorElementType == Float || _vectorElementType == Int32)
      return 4;
   if (_vectorElementType == Int16)
      return 8;
   if (_vectorElementType == Int8)
      return 16;

   // unknown datatype, so just do it as a scalar loop
   return 1;
   }

TR::IlValue *
OMR::VectorLoopBuilder::LoadIterationVar()
   {
   TR::IlValue *residueIterValue = _residueLoopBody.Load(residueIteratorVariable());
   TraceIL("VectorLoopBuilder[ %p ]::LoadIterationVar residueLoop %d", this, residueIterValue->getID());
   if (_hasVectorLoop)
      {
      TR::IlValue *vectorIterValue =  _vectorLoopBody.Load(vectorIteratorVariable());
      _vectorValueMap[residueIterValue] = vectorIterValue;
      TraceIL(" vectorLoop %d\n", this, vectorIterValue->getID());
      }
   else
      TraceIL(" (no vectorLoop) \n");

   return residueIterValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::ConstInt8(int8_t value)
   {
   TR::IlValue *residueConstValue = _residueLoopBody.ConstInt8(value);
   TraceIL("VectorLoopBuilder[ %p ]::ConstInt8 %d into residueLoop %d", this, value, residueConstValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorConstValue = _vectorLoopBody.ConstInt8(value);
      _vectorValueMap[residueConstValue] = vectorConstValue;
      TraceIL(" vectorLoop %d\n", this, vectorConstValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueConstValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::ConstInt16(int16_t value)
   {
   TR::IlValue *residueConstValue = _residueLoopBody.ConstInt16(value);
   TraceIL("VectorLoopBuilder[ %p ]::ConstInt16 %d into residueLoop %d", this, value, residueConstValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorConstValue = _vectorLoopBody.ConstInt16(value);
      _vectorValueMap[residueConstValue] = vectorConstValue;
      TraceIL(" vectorLoop %d\n", this, vectorConstValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueConstValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::ConstInt32(int32_t value)
   {
   TR::IlValue *residueConstValue = _residueLoopBody.ConstInt32(value);
   TraceIL("VectorLoopBuilder[ %p ]::ConstInt32 %d into residueLoop %d", this, value, residueConstValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorConstValue = _vectorLoopBody.ConstInt32(value);
      _vectorValueMap[residueConstValue] = vectorConstValue;
      TraceIL(" vectorLoop %d\n", this, vectorConstValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueConstValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::ConstInt64(int64_t value)
   {
   TR::IlValue *residueConstValue = _residueLoopBody.ConstInt64(value);
   TraceIL("VectorLoopBuilder[ %p ]::ConstInt64 %ld into residueLoop %d", this, value, residueConstValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorConstValue = _vectorLoopBody.ConstInt64(value);
      _vectorValueMap[residueConstValue] = vectorConstValue;
      TraceIL(" vectorLoop %d\n", this, vectorConstValue->getID());
      }
   else
      TraceIL(" (no residue loop)\n");

   return residueConstValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::ConstFloat(float value)
   {
   TR::IlValue *residueConstValue = _residueLoopBody.ConstFloat(value);
   TraceIL("VectorLoopBuilder[ %p ]::ConstFloat %f into residueLoop %d", this, value, residueConstValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorConstValue = _vectorLoopBody.ConstFloat(value);
      _vectorValueMap[residueConstValue] = vectorConstValue;
      TraceIL(" vectorLoop %d\n", this, vectorConstValue->getID());
      }
   else
      TraceIL(" (no residue loop)\n");

   return residueConstValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::ConstDouble(double value)
   {
   TR::IlValue *residueConstValue = _residueLoopBody.ConstDouble(value);
   TraceIL("VectorLoopBuilder[ %p ]::ConstDouble %d into residueLoop %d", this, value, residueConstValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorConstValue = _vectorLoopBody.ConstDouble(value);
      _vectorValueMap[residueConstValue] = vectorConstValue;
      TraceIL(" vectorLoop %d\n", this, vectorConstValue->getID());
      }
   else
      TraceIL(" (no residue loop)\n");

   return residueConstValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::ConstInteger(TR::IlType *intType, int64_t value)
   {
   if      (intType == Int8)  return ConstInt8 ((int8_t)  value);
   else if (intType == Int16) return ConstInt16((int16_t) value);
   else if (intType == Int32) return ConstInt32((int32_t) value);
   else if (intType == Int64) return ConstInt64(          value);

   TR_ASSERT(0, "unknown integer type");
   return NULL;
   }

TR::IlValue *
OMR::VectorLoopBuilder::Add(TR::IlValue *left, TR::IlValue *right)
   {
   TR::IlValue *residueReturnValue = _residueLoopBody.Add(left, right);
   TraceIL("VectorLoopBuilder[ %p ]::Add residueLoop %d = %d + %d ", this, residueReturnValue->getID(), left->getID(), right->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorLeft  = getVectorValue(left);
      TR::IlValue *vectorRight = getVectorValue(right);
      TR::IlValue *vectorReturnValue = _vectorLoopBody.Add(vectorLeft, vectorRight);
      _vectorValueMap[residueReturnValue] = vectorReturnValue;

      TraceIL(" vectorLoop %d = %d + %d\n", this, vectorReturnValue->getID(), vectorLeft->getID(), vectorRight->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueReturnValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::Sub(TR::IlValue *left, TR::IlValue *right)
   {
   TR::IlValue *residueReturnValue = _residueLoopBody.Sub(left, right);
   TraceIL("VectorLoopBuilder[ %p ]::Sub residueLoop %d = %d - %d ", this, residueReturnValue->getID(), left->getID(), right->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorLeft  = getVectorValue(left);
      TR::IlValue *vectorRight = getVectorValue(right);
      TR::IlValue *vectorReturnValue = _vectorLoopBody.Sub(vectorLeft, vectorRight);
      _vectorValueMap[residueReturnValue] = vectorReturnValue;

      TraceIL(" vectorLoop %d = %d - %d\n", this, vectorReturnValue->getID(), vectorLeft->getID(), vectorRight->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueReturnValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::Mul(TR::IlValue *left, TR::IlValue *right)
   {
   TR::IlValue *residueReturnValue = _residueLoopBody.Mul(left, right);
   TraceIL("VectorLoopBuilder[ %p ]::Mul residueLoop %d = %d * %d ", this, residueReturnValue->getID(), left->getID(), right->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorLeft  = getVectorValue(left);
      TR::IlValue *vectorRight = getVectorValue(right);
      TR::IlValue *vectorReturnValue = _vectorLoopBody.Mul(vectorLeft, vectorRight);
      _vectorValueMap[residueReturnValue] = vectorReturnValue;

      TraceIL(" vectorLoop %d = %d * %d\n", this, vectorReturnValue->getID(), vectorLeft->getID(), vectorRight->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueReturnValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::Load(const char *name)
   {
   TR::IlValue *residueValue = _residueLoopBody.Load(name);
   TraceIL("VectorLoopBuilder[ %p ]::Load %s residueLoop into %d", this, name, residueValue->getID());
   if (_hasVectorLoop)
      {
      TR::IlValue *vectorValue = _vectorLoopBody.Load(name);
      _vectorValueMap[residueValue] = vectorValue;

      TraceIL(" vectorLoop into %d\n", this, vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::VectorLoad(const char *name)
   {
   TR::IlValue *residueValue = _residueLoopBody.Load(name);
   TraceIL("VectorLoopBuilder[ %p ]::VectorLoad residueLoop %s into %d", this, name, residueValue->getID());

   if (_hasVectorLoop)
      {
      char *vectorName = getVectorName(name);
      TR::IlValue *vectorValue = _vectorLoopBody.VectorLoad(vectorName);
      _vectorValueMap[residueValue] = vectorValue;

      TraceIL(" vectorLoop %s into %d\n", this, vectorName, vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueValue;
   }

void
OMR::VectorLoopBuilder::Store(const char *name, TR::IlValue *value)
   {
   _residueLoopBody.Store(name, value);
   TraceIL("VectorLoopBuilder[ %p ]::Store %s gets residueLoop %d", this, name, value->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorValue = getVectorValue(value);
      _vectorLoopBody.Store(name, vectorValue);
      TraceIL(" vectorLoop %d\n", vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");
   }

void
OMR::VectorLoopBuilder::VectorStore(const char *name, TR::IlValue *value)
   {
   _residueLoopBody.Store(name, value);
   TraceIL("VectorLoopBuilder[ %p ]: residueLoop VectorStore %s gets %d", this, name, value->getID());

   if (_hasVectorLoop)
      {
      char *vectorName = getVectorName(name);
      TR::IlValue *vectorValue = getVectorValue(value);
      _vectorLoopBody.VectorStore(vectorName, vectorValue);
      TraceIL(" vectorLoop Store %s gets %d\n", vectorName, vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");
   }

TR::IlValue *
OMR::VectorLoopBuilder::ArrayLoad(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index)
   {
   TR::IlValue *residueValue = _residueLoopBody.ArrayLoad(dt, base, index);
   TraceIL("VectorLoopBuilder[ %p ]::ArrayLoad type %s residueLoop base %d [ index %d ] into %d", this, dt->getName(), base->getID(), index->getID(), residueValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorBase = getVectorValue(base);
      TR::IlValue *vectorIndex = getVectorValue(index);
      TR::IlValue *vectorValue = _vectorLoopBody.ArrayLoad(dt, vectorBase, vectorIndex);
      _vectorValueMap[residueValue] = vectorValue;

      TraceIL(" vectorLoop base %d [ index %d ] into %d\n", vectorBase->getID(), vectorIndex->getID(), vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueValue;
   }

TR::IlValue *
OMR::VectorLoopBuilder::VectorArrayLoad(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index)
   {
   TR::IlValue *residueValue = _residueLoopBody.ArrayLoad(dt, base, index);
   TraceIL("VectorLoopBuilder[ %p ]::VectorArrayLoad type %s residueLoop base %d [ index %d ] into %d", this, dt->getName(), base->getID(), index->getID(), residueValue->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorBase = getVectorValue(base);
      TR::IlValue *vectorIndex = getVectorValue(index);
      TR::IlValue *vectorValue = _vectorLoopBody.VectorArrayLoad(dt, vectorBase, vectorIndex);
      _vectorValueMap[residueValue] = vectorValue;
      TraceIL(" vectorLoop base %d [ index %d ] into %d\n", vectorBase->getID(), vectorIndex->getID(), vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");

   return residueValue;
   }

void
OMR::VectorLoopBuilder::ArrayStore(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index, TR::IlValue *value)
   {
   _residueLoopBody.ArrayStore(dt, base, index, value);
   TraceIL("VectorLoopBuilder[ %p ]::ArrayStore type %s residueLoop base %d [ index %d ] gets %d", this, dt->getName(), base->getID(), index->getID(), value->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorBase = getVectorValue(base);
      TR::IlValue *vectorIndex = getVectorValue(index);
      TR::IlValue *vectorValue = getVectorValue(value);
      _vectorLoopBody.ArrayStore(dt, vectorBase, vectorIndex, vectorValue);

      TraceIL(" vectorLoop base %d [ index %d ] gets %d\n", this, dt->getName(), vectorBase->getID(), vectorIndex->getID(), vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");
   }

void
OMR::VectorLoopBuilder::VectorArrayStore(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index, TR::IlValue *value)
   {
   _residueLoopBody.ArrayStore(dt, base, index, value);
   TraceIL("VectorLoopBuilder[ %p ]::VectorArrayStore type %s residueLoop base %d [ index %d ] gets %d", this, dt->getName(), base->getID(), index->getID(), value->getID());

   if (_hasVectorLoop)
      {
      TR::IlValue *vectorBase = getVectorValue(base);
      TR::IlValue *vectorIndex = getVectorValue(index);
      TR::IlValue *vectorValue = getVectorValue(value);
      _vectorLoopBody.VectorArrayStore(dt, vectorBase, vectorIndex, vectorValue);
      TraceIL(" vectorLoop base %d [ index %d ] gets %d\n", this, dt->getName(), vectorBase->getID(), vectorIndex->getID(), vectorValue->getID());
      }
   else
      TraceIL(" (no vector loop)\n");
}

TR::VectorLoopBuilder *
OMR::VectorLoopBuilder::ForLoop(TR::IlValue *initial,
                                TR::IlValue *end,
                                TR::IlValue *increment)
   {
   TR::VectorLoopBuilder *loopBuilder = orphanVectorLoopBuilder(_vectorElementType);
   loopBuilder->setParentVectorLoopBuilder(static_cast<TR::VectorLoopBuilder *>(this));

   TR::IlBuilder *residueLoopBody = loopBuilder->residueBody();
   char *residueIter = loopBuilder->residueIteratorVariable();
   _residueLoopBody.ForLoop(true, residueIter, &residueLoopBody, NULL, NULL, initial, end, increment);
   TraceIL("VectorLoopBuilder[ %p ]::ForLoop from %d to %d by %d residueLoop [ %p ] using %s", this, initial->getID(), end->getID(), increment->getID(), residueLoopBody, residueIter);

   if (_hasVectorLoop)
      {
      TR::IlBuilder *vectorLoopBody = loopBuilder->vectorBody();
      char *vectorIter = loopBuilder->vectorIteratorVariable();
      _vectorLoopBody.ForLoop(true, vectorIter, &vectorLoopBody, NULL, NULL, initial, end, increment);

      TraceIL(" vectorLoop [ %p ] using %s", vectorLoopBody, vectorIter);
      }
   else
      TraceIL(" (no vector loop)\n");

   return loopBuilder;
   }
