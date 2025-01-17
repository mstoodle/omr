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

#ifndef COREEXTENSION_INCL
#define COREEXTENSION_INCL

#include "common.hpp"
#include "Extension.hpp"

namespace OMR {
namespace JB2 {

class Compilation;
class CompileUnit;
class Config;
class IR;
class NoTypeType;

class CoreExtension : public Extension {
    JBALLOC_(CoreExtension)

public:
    DYNAMIC_ALLOC_ONLY(CoreExtension, LOCATION, Compiler *compiler);

    virtual const SemanticVersion * semver() const { return &version; }
    static const String NAME;

protected:
    // needs to precede _strategyCodegen
    Strategy *_codegenStrategy;
    Pass *_dispatcher;

public:
    // Core Types
    //
    const TypeID tNoType;

    //
    // Core actions
    //
    const ActionID aAppendBuilder;
    const ActionID aMergeDef;

    //
    // Core strategies
    //
    const StrategyID strategyCodegen;

    // 
    // Core types
    //
    const NoTypeType *NoType(IR *ir);

    // 
    // Core operations
    //
    void AppendBuilder(LOCATION, Builder *parent, Builder *b);
    void MergeDef(LOCATION, Builder *parent, Value *existingDef, Value *newDef);

    //
    // Compiler return codes
    //
    const CompilerReturnCode CompileFail_CodeGeneratorMissingOperationHandler;

protected:
    static const SemanticVersion version;

public:
    // depends on _compiler being initialized

    SUBCLASS_KINDSERVICE_DECL(Extensible, CoreExtension);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(COREEXTENSION_INCL)
