/*******************************************************************************
 * Copyright (c) 2014, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "api/omrgen/OMRIlGen.hpp"
#include "compile/Compilation.hpp"
#include "compile/JB2ResolvedMethod.hpp"
#include "il/ParameterSymbol.hpp"
#include "il/SymbolReference.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "ilgen/IlInjector.hpp"
#include "ilgen/IlBuilder.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/IlGeneratorMethodDetails_inlines.hpp"

//#include "Compilation.hpp"

namespace OMR {
namespace JitBuilder {
namespace omrgen {

const char *
JB2ResolvedMethod::signature(TR_Memory * trMemory, TR_AllocationKind allocKind)
   {
   if( !_signature )
      {
      size_t len= strlen(_fileName) + 1 + strlen(_lineNumber) + 1 + strlen(_name) + 1;
      char * s = (char *)trMemory->allocateMemory(len, allocKind);
      snprintf(s, len, "%s:%s:%s", _fileName, _lineNumber, _name);

      if ( allocKind == heapAlloc)
        _signature = s;

      return s;
      }
   else
      return _signature;
   }

const char *
JB2ResolvedMethod::externalName(TR_Memory *trMemory, TR_AllocationKind allocKind)
   {
   if( !_externalName)
      {
      // For C++, need to mangle name
      //char * s = (char *)trMemory->allocateMemory(1 + strlen(_name) + 1, allocKind);
      //sprintf(s, "_Z%d%si", (int32_t)strlen(_name), _name);


      // functions must be defined as extern "C"
      _externalName = _name;

      //if ( allocKind == heapAlloc)
      //  _externalName = s;
      }

   return _externalName;
   }

TR::DataType
JB2ResolvedMethod::parmType(uint32_t slot)
   {
   TR_ASSERT((slot < unsigned(_numParms)), "Invalid slot provided for Parameter Type");
   return _parmTypes[slot];
   }

void
JB2ResolvedMethod::computeSignatureChars()
   {
   _signatureChars[0] = 0;
   }


char *
JB2ResolvedMethod::localName(uint32_t slot,
                          uint32_t bcIndex,
                          int32_t &nameLength,
                          TR_Memory *trMemory)
   {
   char * name = (char *) trMemory->allocateHeapMemory(strlen(_parmNames[slot]));
   strcpy(name, _parmNames[slot]);
   nameLength = static_cast<int32_t>(strlen(name));
   return name;
   }

TR_IlGenerator *
JB2ResolvedMethod::getIlGenerator (TR::IlGeneratorMethodDetails * details,
   TR::ResolvedMethodSymbol *methodSymbol,
   TR_FrontEnd *fe,
   TR::SymbolReferenceTable *symRefTab)
   {
   static_cast<OMRIlGen *>(_ilgen)->initialize(details, methodSymbol, fe, symRefTab);
   return _ilgen;
   }

TR::DataType
JB2ResolvedMethod::returnType()
   {
   return _returnType;
   }

const char *
JB2ResolvedMethod::signatureNameForType[TR::NumOMRTypes] =
   {
   "V",  // NoType
   "B",  // Int8
   "C",  // Int16
   "I",  // Int32
   "J",  // Int64
   "F",  // Float
   "D",  // Double
   "L",  // Address
   "A"   // Aggregate
   };

const char *
JB2ResolvedMethod::signatureNameForVectorType[TR::NumVectorElementTypes] =
   {
   "V1", // VectorInt8
   "V2", // VectorInt16
   "V4", // VectorInt32
   "V8", // VectorInt64
   "VF", // VectorFloat
   "VD", // VectorDouble
   };

const char *
JB2ResolvedMethod::signatureNameForMaskType[TR::NumVectorElementTypes] =
   {
   "M1", // MaskInt8
   "M2", // MaskInt16
   "M4", // MaskInt32
   "M8", // MaskInt64
   "MF", // MaskFloat
   "MD", // MaskDouble
   };

char *
JB2ResolvedMethod::getParameterTypeSignature(int32_t parmIndex)
   {
   return ""; //_parmTypes[parmIndex]->getSignatureName();
   }

} // namespace omrgen
} // namespace JitBuilder
} // namespace OMR

