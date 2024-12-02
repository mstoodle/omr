/*******************************************************************************
 * Copyright (c) 2014, 2020 IBM Corp. and others
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

#include <stdio.h>
#include "codegen/CodeGenerator.hpp"
#include "compile/CompilationTypes.hpp"
#include "compile/Method.hpp"
#include "control/CompilationController.hpp"
#include "control/CompileMethod.hpp"
#include "env/CompilerEnv.hpp"
#include "env/FrontEnd.hpp"
#include "env/IO.hpp"
#include "env/RawAllocator.hpp"
#include "ilgen/IlGeneratorMethodDetails_inlines.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "runtime/CodeCache.hpp"
#include "runtime/Runtime.hpp"
#include "runtime/JBJitConfig.hpp"

#if defined(AIXPPC)
#include "p/codegen/PPCTableOfConstants.hpp"
#endif

int32_t
compile(TR::MethodBuilder *m, void **entry, TR_Hotness level)
   {
   JB2ResolvedMethod resolvedMethod(static_cast<TR::MethodBuilder *>(m));
   IlGeneratorJB2MethodDetails details(&resolvedMethod);

   //TR_Hotness level=warm;
   int32_t rc=0;
   *entry = (void *) compileMethodFromDetails(NULL, details, level, rc);

#if defined(AIXPPC)
   struct FunctionDescriptor
      {
      void* func;
      void* toc;
      void* environment;
      };

   FunctionDescriptor* fd = new FunctionDescriptor();
   fd->func = *entry;
   // TODO: There should really be a better way to get this. Usually, we would use
   // cg->getTOCBase(), but the code generator has been long destroyed by now...
   fd->toc = toPPCTableOfConstants(TR_PersistentMemory::getNonThreadSafePersistentInfo()->getPersistentTOC())->getTOCBase();
   fd->environment = NULL;

   *entry = (uint8_t*) fd;
#endif

   return rc;
   }

