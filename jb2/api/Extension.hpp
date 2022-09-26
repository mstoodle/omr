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

#ifndef EXTENSION_INCL
#define EXTENSION_INCL

#include <string>
#include <vector>
#include "CreateLoc.hpp"
#include "IDs.hpp"
#include "Type.hpp" // for TypeKind

namespace OMR {
namespace JitBuilder {

class Builder;
class Compiler;
class Context;
class Operation;
class Pass;
class SemanticVersion;
class TypeDictionary;
class Value;

class Extension {
    friend class Compiler;
    friend class Operation;

public:
    virtual const SemanticVersion * semver() const { return &version; }

    Extension(Compiler *compiler, std::string name);
    Compiler *compiler() const { return _compiler; }
    std::string name() const { return _name; }

    const std::string actionName(ActionID a) const;

    // 
    // Core operations
    //

    void MergeDef(LOCATION, Builder *parent, Value *existingDef, Value *newDef);

    //
    // Core pseudo operations
    //

    Builder *OrphanBuilder(LOCATION, Builder *parent, Context *context=NULL, std::string name="");
    Builder *BoundBuilder(LOCATION, Builder *parent, Operation *parentOp, std::string name="");

protected:
    ActionID registerAction(std::string name);
    CompilerReturnCode registerReturnCode(std::string name);
    PassID addPass(Pass *pass); 

    Value *createValue(const Builder *parent, const Type *type);
    void addOperation(Builder *b, Operation *op);

    virtual uint64_t numTypes() const { return static_cast<uint64_t>(_types.size()); }

    ExtensionID _id;
    std::string _name;
    Compiler *_compiler;
    std::vector<const Type *> _types;

    static const SemanticVersion version;
    
public:
    // depends on _compiler being initialized

    //
    // Core actions
    //

    const ActionID aMergeDef;

};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(EXTENSION_INCL)
