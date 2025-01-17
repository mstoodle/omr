/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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

#ifndef OMR_JITBUILDER_JBCORE_INCL
#define OMR_JITBUILDER_JBCORE_INCL

#include "common.hpp"
//#include "Addon.hpp" // must be included via Extensible.hpp
#include "AddonIR.hpp"
#include "AllocatorTracer.hpp"
#include "AllocatorTracker.hpp"
#include "Builder.hpp"
#include "BuilderEntry.hpp"
#include "CodeGenerator.hpp"
#include "CodeGeneratorExtensionAddon.hpp"
#include "CodeGeneratorForCore.hpp"
#include "CodeGeneratorForExtension.hpp"
#include "Compilation.hpp"
#include "CompiledBody.hpp"
#include "Compiler.hpp"
#include "CompileUnit.hpp"
#include "Config.hpp"
#include "Context.hpp"
#include "CoreExtension.hpp"
#include "CreateLoc.hpp"
#include "EntryPoint.hpp"
#include "Extensible.hpp"
#include "ExtensibleIR.hpp"
#include "Extension.hpp"
#include "InputReader.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"
#include "KindService.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "Location.hpp"
#include "Loggable.hpp"
#include "Mapper.hpp"
#include "NativeEntry.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "OperationReplacer.hpp"
#include "Pass.hpp"
#include "Scope.hpp"
#include "SemanticVersion.hpp"
#include "Strategy.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "TextWriter.hpp"
#include "Transformer.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "TypeReplacer.hpp"
#include "Value.hpp"
#include "Visitor.hpp"
#include "String.hpp"

#if defined(OSX)
    #define OMR_JB2_CORELIB "libjb2.dylib"
#else
    #define OMR_JB2_CORELIB "libjb2.so"
#endif

#endif // defined(OMR_JITBUILDER_JBCORE_INCL)

