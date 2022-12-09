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

#include <vector>
#include "CreateLoc.hpp"
#include "IDs.hpp"
#include "Type.hpp" // for TypeKind
#include "typedefs.hpp"
#include "util/String.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Compiler;
class CompileUnit;
class Context;
class Location;
class NoTypeType;
class Operation;
class Pass;
class SemanticVersion;
class TypeDictionary;
class Value;

class Extension {
    friend class Compiler;
    friend class Context;
    friend class Operation;

public:
    virtual const SemanticVersion * semver() const { return &version; }
    static const String NAME;

    Extension(LOCATION, Compiler *compiler);

    Compiler *compiler() const { return _compiler; }
    String name() const { return _name; }

    const String actionName(ActionID a) const;

protected:
    ExtensionID _id;
    String _name;
    Compiler *_compiler;
    CreateLocation _createLoc;
    std::vector<const Type *> _types;

public:
    // 
    // Core types
    //

    const NoTypeType *NoType;

    // 
    // Core operations
    //

    void MergeDef(LOCATION, Builder *parent, Value *existingDef, Value *newDef);

    //
    // Core pseudo operations
    //

    Builder *BoundBuilder(LOCATION, Builder *parent, Operation *parentOp, String name="");
    Builder *OrphanBuilder(LOCATION, Builder *parent, Context *context=NULL, String name="");
    Location * SourceLocation(LOCATION, Builder *b, String func);
    Location * SourceLocation(LOCATION, Builder *b, String func, String lineNumber);
    Location * SourceLocation(LOCATION, Builder *b, String func, String lineNumber, int32_t bcIndex);

protected:
    Extension(LOCATION, Compiler *compiler, String name);

    ActionID registerAction(String name);
    CompilerReturnCode registerReturnCode(String name);
    PassID addPass(Pass *pass); 

    Value *createValue(const Builder *parent, const Type *type);
    void addOperation(Builder *b, Operation *op);

    virtual uint64_t numTypes() const { return static_cast<uint64_t>(_types.size()); }

    Builder *EntryBuilder(LOCATION, Compilation *comp, Context *context, String name="");
    Builder *ExitBuilder(LOCATION, Compilation *comp, Context *context, String name="");

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
