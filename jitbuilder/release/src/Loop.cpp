/*******************************************************************************
 * Copyright (c) 2016, 2018 IBM Corp. and others
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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "Jit.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/VectorLoopBuilder.hpp"
#include "Loop.hpp"

static void printString(int64_t ptr)
   {
   #define PRINTSTRING_LINE LINETOSTR(__LINE__)
   char *str = (char *) ptr;
   printf("%s", str);
   }

static void printPointer(int64_t val)
   {
   #define PRINTPOINTER_LINE LINETOSTR(__LINE__)
   printf("%llx", val);
   }

Loop::Loop(TR::TypeDictionary *types)
   : MethodBuilder(types)

   {
   pInt32 = types->PointerTo(Int32);

   DefineLine(LINETOSTR(__LINE__));
   DefineFile(__FILE__);

   DefineName("vector_multiply");

   DefineParameter("result", pInt32);
   DefineParameter("vector1", pInt32);
   DefineParameter("vector2", pInt32);
   DefineParameter("length", Int32);

   DefineReturnType(NoType);

   DefineFunction((char *)"printString", 
                  (char *)__FILE__,
                  (char *)PRINTSTRING_LINE,
                  (void *)&printString,
                  NoType,
                  1,
                  Int64);
   DefineFunction((char *)"printPointer", 
                  (char *)__FILE__,
                  (char *)PRINTPOINTER_LINE,
                  (void *)&printPointer,
                  NoType,
                  1,
                  Int64);

   }

void
Loop::PrintString(TR::IlBuilder *bldr, const char *s)
   {
   bldr->Call("printString", 1,
   bldr->   ConstInt64((int64_t)(char *)s));
   }

bool
Loop::buildIL()
   {
   PrintString(this, "multiply parameters:\n");

   PrintString(this, "   result is ");
   Call("printPointer", 1,
      Load("result"));
   PrintString(this, "\n");

   PrintString(this, "   vector1 is ");
   Call("printPointer", 1,
      Load("vector1"));
   PrintString(this, "\n");

   PrintString(this, "   vector2 is ");
   Call("printPointer", 1,
      Load("vector2"));
   PrintString(this, "\n");

   TR::VectorLoopBuilder *loop = VectorForLoop(Int32, ConstInt32(0), Load("length"));

   loop->VectorArrayStore(pInt32,
   loop->   Load("result"),
   loop->   LoadIterationVar(),
   loop->   Mul(
   loop->      VectorArrayLoad(pInt32,
   loop->         Load("vector1"),
   loop->         LoadIterationVar()),
   loop->      VectorArrayLoad(pInt32,
   loop->         Load("vector2"),
   loop->         LoadIterationVar())));

   Return();

   return true;
   }


int
main(int argc, char *argv[])
   {
   printf("Step 1: initialize JIT\n");
   bool initialized = initializeJit();
   if (!initialized)
      {
      fprintf(stderr, "FAIL: could not initialize JIT\n");
      exit(-1);
      }

   printf("Step 2: define type dictionary\n");
   TR::TypeDictionary types;

   printf("Step 3: compile method builder\n");
   Loop method(&types);
   uint8_t *entry=0;
   int32_t rc = compileMethodBuilder(&method, &entry);
   if (rc != 0)
      {
      fprintf(stderr,"FAIL: compilation error %d\n", rc);
      exit(-2);
      }

   printf("Step 4: define values\n");
   const int32_t N=19;
   int32_t result[N] = { 0 };
   int32_t values1[N] = {  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19 };
   int32_t values2[N] = { 18,17,16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1,19 };

   printf("Step 5: invoke compiled code and verify results\n");
   LoopFunctionType *test = (LoopFunctionType *)entry;
   test(result, values1, values2, N);

   printf("result = [\n");
   for (int32_t i=0;i < N;i++)
      printf("           %d\n", (int)result[i]);
   printf("         ]\n\n");

   printf ("Step 6: shutdown JIT\n");
   shutdownJit();

   printf("PASS\n");
   }
