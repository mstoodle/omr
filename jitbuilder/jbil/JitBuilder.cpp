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

#include "JitBuilder.hpp"
#include "CodeGenerator.hpp"
#include "PrettyPrinter.hpp"

#include "ilgen/MethodBuilder.hpp"
#include "ilgen/TypeDictionary.hpp"

bool internal_initializeJit();
int32_t internal_compileMethodBuilder(TR::MethodBuilder * methodBuilder, void ** entryPoint);
void internal_shutdownJit();


bool
initializeJit()
   {
   internal_initializeJit();
   return true;
   }

bool
constructFunctionBuilder(OMR::JitBuilder::FunctionBuilder * fb)
   {
   OMR::JitBuilder::PrettyPrinter * logger = fb->logger();
   if (logger)
      {
      (*logger) << "Logging constructFunctionBuilder:\n";
      (*logger) << fb << " : call buildIL()\n";
      }
   return fb->constructIL();
   }

class CompileMethodBuilder : public TR::MethodBuilder
   {
   public:
   CompileMethodBuilder(OMR::JitBuilder::FunctionBuilder *fb, TR::TypeDictionary *types)
      : TR::MethodBuilder(types)
      , _fb(fb)
      , _cg(fb, this)
      {
      _cg.generateFunctionAPI(fb);
      }

   virtual bool buildIL()
      {
      bool success = _fb->ilBuilt();
      if (!success)
         success = _fb->buildIL();

      if (success)
         _cg.transform();

      return success;
      }

   protected:
   OMR::JitBuilder::FunctionBuilder * _fb;
   OMR::JitBuilder::CodeGenerator _cg;
   };

int32_t
compileFunctionBuilder(OMR::JitBuilder::FunctionBuilder *fb, void **entry)
   {
   TR::TypeDictionary types;
   CompileMethodBuilder cmb(fb, &types);
   return internal_compileMethodBuilder(&cmb, entry);
   }

void
shutdownJit()
   {
   internal_shutdownJit();
   }
