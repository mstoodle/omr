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

#ifndef VMEXTENSION_INCL
#define VMEXTENSION_INCL

#include <stdint.h>
#include <map>

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"


namespace OMR {
namespace JitBuilder {
namespace VM {

class VMExtension : public Extension {
    friend class VMExtensionChecker;

public:
    VMExtension(LOCATION, Compiler *compiler, bool extended=false, std::string extensionName="vm");
    virtual ~VMExtension();

    static const std::string NAME;

    virtual const SemanticVersion * semver() const {
        return &version;
    }

    Func::FunctionExtension *fx() const { return _fx; }
    Base::BaseExtension *bx() const { return _bx; }

    //
    // Types
    //

    //
    // Actions
    //

    //
    // CompilerReturnCodes
    //

    //
    // Operations
    //

    // Pseudo operations
    void Goto(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target);
    void IfCmpEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpEqualZero(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *condition);
    void IfCmpLessOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpLessThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpGreaterOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpGreaterThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpNotEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpNotEqualZero(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *condition);
    void IfCmpUnsignedLessOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpUnsignedLessThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpUnsignedGreaterOrEqual(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    void IfCmpUnsignedGreaterThan(LOCATION, BytecodeBuilder *b, BytecodeBuilder *target, Value *left, Value *right);
    BytecodeBuilder *OrphanBytecodeBuilder(Base::BaseCompilation *comp, int32_t bcIndex, int32_t bcLength=1, Context *context=NULL, std::string name="");

protected:
    Base::BaseExtension *_bx;
    Func::FunctionExtension *_fx;

    static const MajorID VMEXT_MAJOR=0;
    static const MinorID VMEXT_MINOR=1;
    static const PatchID VMEXT_PATCH=0;
    static const SemanticVersion version;
};

} // namespace VM
} // namespace JitBuilder
} // namespace OMR

#endif // defined(VMEXTENSION_INCL)
