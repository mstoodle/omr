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

#ifndef CODEGENERATOR_INCL
#define CODEGENERATOR_INCL

#include "common.hpp"
#include "KindService.hpp"
#include "Loggable.hpp"
#include "String.hpp"
#include "Transformer.hpp"

namespace OMR {
namespace JB2 {

class Builder;
class Context;
class Extension;
class Literal;
class Operation;
class Scope;
class Symbol;
class Type;
class Value;

// CodeGenerator objects are created by Extensions (in the case where the specific
// type of code generator is introduced by the Extension) or are registered
// against an Extension (when an Extension wants to add support for a particular
// kind of code generator for the Operations defined by another Extension).
//
// The primary CodeGenerator pass added to a Strategy will be the first kind,
// but when the primary CodeGenerator traverses the Operations in the IL, it will
// delegate to second kind of CodeGenerator objects which should be registered
// for the Extension that owns the Operation. If an Operation is encountered
// whose Extension object does not have a CodeGenerator or that CodeGenerator
// does not handle the Operation, then the primary CodeGenerator will attempt
// to generate code for the Operation. If even the primary CodeGenerator cannot
// handle the Operation, then the pass will return a failure and the compilation
// will fail. This mechanism for extension permits even a third Extension to
// extend the behaviour of a primary CodeGenerator object for another Extension's
// Operations without either the primary or the Operation Extension knowing anything
// specifically about each other or the third Extension. Any conflict (i.e. if there
// are multiple Extension objects that try to extend the same kind of CodeGenerator
// for a particular Extension) must be negotiated when registering CodeGenerator
// objects against an Extension. An Extension is allowed to have multiple kinds
// of registered CodeGenerator objects, but there can be only one CodeGenerator
// object of a particular kind.
// Fine-grained conflicts can be managed by creating a new kind of CodeGenerator
// whose purpose is to decide, for a specific Operation, how to deal appropriately
// with that Operation's multiple CodeGenerator objects. In the example above,
// a hypothetical combined DifferentialJB1CodeGenerator could hold both "older"
// JB1CodeGenerator and "newer" JB1CodeGenerator objects which could generate code
// into two different code cache to enable analysis or comparison of the "old"
// versus "new" code.

// A Strategy mue a specific CodeGenerator object, but different Strategy
// objects could use different CodeGenerator objects. For example, a ColdStrategy
// could contain a SimpleCodeGenerator object whereas a HotStrategy could contain
// a SmarterCodeGenerator object. Another example could be a JBCompile strategy
// that utilizes a JBCodeGenerator object while an LLVMCompile strategy could
// utlize an LLVMCodeGenerator. In some cases, however, it doesn't matter what
// CodeGenerator is used, only that code will be generated. For this scenario, the
// Compiler object maintains a "registry" that records a "canonical" CodeGenerator
// object. The Compiler can be queried for this canonical object so it can be placed
// into another Strategy.

class CodeGenerator : public Visitor {
    JBALLOC_(CodeGenerator)

public:
    DYNAMIC_ALLOC_ONLY(CodeGenerator, KINDTYPE(Extensible) kind, Extension *ext, String name);

    // handle an operation
    virtual Builder * gencode(Operation *op) { return NULL; }

    // none of these are mandatory but can be overriden and will be called before traversing any Operations
    virtual bool registerBuilder(Builder *b)              { return true; }
    virtual bool registerContext(Context *c)              { return true; }
    virtual bool registerLiteral(Literal *lv)             { return true; }
    virtual bool registerScope(Scope *s)                  { return true; }
    virtual bool registerSymbol(Symbol *sym)              { return true; }
    virtual bool registerType(const Type *type)           { return true; }
    virtual bool registerValue(Value *value)              { return true; }

protected:
    virtual void visitBegin()                             { }
    virtual void visitPreCompilation(Compilation * comp)  { }
    virtual void visitPostCompilation(Compilation * comp) { }
    virtual void visitBuilderPreOps(Builder * b)          { }
    virtual void visitBuilderPostOps(Builder * b)         { }
    virtual void visitOperation(Operation * op)           { }
    virtual void visitEnd()                               { }

    // probably more to go in here?
    virtual Builder * transformOperation(Operation * op) { return gencode(op); }

    Extension *_ext;

    SUBCLASS_KINDSERVICE_DECL(Extensible,CodeGenerator);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(CODEGENERATOR_INCL)
