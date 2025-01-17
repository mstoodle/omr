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

#include "common.hpp"
#include "Compilation.hpp"
#include "CreateLoc.hpp"
#include "Extensible.hpp"
#include "Pass.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Builder;
class Compilation;
class Compiler;
class CompileUnit;
class Context;
class IR;
class Location;
class Operation;
class Scope;
class SemanticVersion;
class Strategy;
class TypeDictionary;
class Value;

class Extension : public Extensible {
    JBALLOC_(Extension)

    friend class Compiler;
    friend class Context;
    friend class Operation;

public:
    virtual const SemanticVersion * semver() const { return &version; }
    static const String NAME;

    Compiler *compiler() const { return _compiler; }
    const String & name() const { return _name; }

    const String actionName(ActionID a) const;

    template <class T>
    T *extendedPass() {
        auto it = _extendedPasses.find(CLASSKIND(T,Extensible));
        if (it == _extendedPasses.end())
            return NULL;
        Pass *extPass = it->second;
        return extPass->refine<T>();
    }

    Builder *BoundBuilder(LOCATION, Builder *parent, Operation *parentOp, String name="");
    Builder *EntryBuilder(LOCATION, Compilation *comp, Scope *scope, String name="") {
        return EntryBuilder(PASSLOC, comp->ir(), scope, name);
    }
    Builder *EntryBuilder(LOCATION, IR *ir, Scope *scope, String name="");
    Builder *ExitBuilder(LOCATION, Compilation *comp, Scope *scope, String name="") {
        return ExitBuilder(PASSLOC, comp->ir(), scope, name);
    }
    Builder *ExitBuilder(LOCATION, IR *ir, Scope *scope, String name="");
    Builder *OrphanBuilder(LOCATION, Builder *parent, Scope *scope=NULL, String name="");
    Location * SourceLocation(LOCATION, Builder *b, String func);
    Location * SourceLocation(LOCATION, Builder *b, String func, String lineNumber);
    Location * SourceLocation(LOCATION, Builder *b, String func, String lineNumber, int32_t bcIndex);

    // register extension to be notified (by createAddon) when Extensible objects of the given kind are created by this Extension
    void registerForExtensible(ExtensibleKind kind, Extension *ext);

protected:
    DYNAMIC_ALLOC_ONLY(Extension, LOCATION, KINDTYPE(Extensible) kind, Compiler *compiler, String name);
    void setContext(Compilation *comp, Context *context);
    void setScope(Compilation *comp, Scope *scope);
    void setLogger(Compilation *comp, TextLogger *logger);

    ExtensionID _id;
    String _name;
    Compiler *_compiler;
    CreateLocation _createLoc;
    Array<const Type *> _types;
    
    Strategy *_codegenStrategy;

    // Other extensions may register passes that provide support for this Extension's elements (Builders, Literals, Operations, Symbols, Types, other)
    std::map<ExtensibleKind, Pass *> _extendedPasses; // should be folded into addons

    // subclasses can override to be notified whenever a new Extension is loaded into the same Compiler
    virtual void notifyNewExtension(Extension *other) { }

    // will be called on Extensible objects for which this Extension object has registered
    virtual void createAddon(Extensible *e) { }

    static void registerExtendedPass(Extension *ext, KINDTYPE(Extensible) kind, Pass *extendedPass);

    const ActionID registerAction(String name) const;
    const CompilerReturnCode registerReturnCode(String name) const;
    const TypeID registerType() const;

    const PassID addPass(Pass *pass); 

    Value *createValue(Builder *parent, const Type *type);
    void addOperation(Builder *b, Operation *op);

    void registerBuilder(IR *ir, Builder *b);

    virtual uint64_t numTypes() const { return static_cast<uint64_t>(_types.length()); }

    static const SemanticVersion version;

    SUBCLASS_KINDSERVICE_DECL(Extensible,Extension);
};

extern "C" {
    typedef Extension * (*CreateFunction)(LOCATION, Compiler *);
}

} // namespace JB2
} // namespace OMR

#endif // !defined(EXTENSION_INCL)
