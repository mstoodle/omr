/*******************************************************************************
 * Copyright IBM Corp. and others 2014
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
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef SIMPLEJIT_INCL
#define SIMPLEJIT_INCL

#include "stdint.h"
#include "compile/CompilationTypes.hpp"

// An individual program should link statically against the compiler, then call:
//     initializeSimpleJit() or initializeSimpleJitWithOptions() to initialize the Jit
//     compile as many times as needed to create compiled code
//     run the compiled code as needed
//     shuwdownJit() when the test is complete (at which time compiled code will be freed)
//

namespace TR { class IlGeneratorMethodDetails; }

extern "C"
{

bool initializeSimpleJitWithOptions(char *options);
bool initializeSimpleJit();
uint8_t *compileMethod(TR::IlGeneratorMethodDetails & details, TR_Hotness hotness, int32_t &rc);
void shutdownSimpleJit();

} // extern "C"


#endif // defined(SIMPLEJIT_INCL)
