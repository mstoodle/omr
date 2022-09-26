/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
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

#ifndef IDENTIFIERTYPES_INCL
#define IDENTIFIERTYPES_INCL

namespace OMR {
namespace JitBuilder {

typedef uint64_t ActionID;
const ActionID NoAction=0;

typedef uint64_t BuilderID;
const BuilderID NoBuilder=0;

typedef int32_t ByteCodeIndex;
const ByteCodeIndex InvalidByteCodeIndex=-1;

typedef uint64_t CaseID;
const CaseID NoCase=0;

typedef uint64_t CompilationID;
const CompilationID NoCompilation=0;

typedef uint64_t CompilerID;
const CompilerID NoCompiler=0;

typedef uint64_t CompilerReturnCode;
// zero is not "special"

typedef uint64_t ContextID;
const ContextID NoContext=0;

typedef uint64_t ExtensionID;
const ExtensionID NoExtension=0;

typedef uint64_t KindServiceID;
// zero is not "special"

typedef uint64_t LiteralID;
const LiteralID NoLiteral=0;

typedef uint64_t LiteralDictionaryID;

typedef uint64_t LocationID;
const LocationID NoLocation=0;

typedef uint64_t OperationID;
const OperationID NoOperation=0;

typedef uint64_t PassID;
const PassID NoPass=0;

typedef uint64_t StrategyID;
const StrategyID NoStrategy=0;

typedef uint64_t SymbolID;
const SymbolID NoSymbol=0;

typedef uint64_t SymbolDictionaryID;

typedef uint64_t TransformationID;
const TransformationID NoTransformation=0;

typedef uint64_t TypeID;
const TypeID NoType=0;

typedef uint64_t TypeDictionaryID;

typedef uint64_t ValueID;
const ValueID NoValue=0;

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(IDENTIFIERTYPES_INCL)

