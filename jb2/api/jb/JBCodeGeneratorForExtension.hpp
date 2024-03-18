/*******************************************************************************
 * Copyright (c) 2024 IBM Corp. and others
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

#ifndef CODEGENERATORFOREXTENSION_INCL
#define CODEGENERATORFOREXTENSION_INCL

#include "common.hpp"
#include "KindService.hpp"
#include "Loggable.hpp"
#include "String.hpp"
#include "Transformer.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Context;
class Extension;
class Literal;
class Operation;
class Scope;
class Symbol;
class Type;
class Value;

class CodeGeneratorForExtension : public Loggable {
    JBALLOC_(CodeGeneratorForExtension)

public:
    DYNAMIC_ALLOC_ONLY(CodeGeneratorForExtension, CodeGenerator *cg, KINDTYPE(Extensible) kind, Extension *ext, String name);

    // handle an operation
    virtual Builder * gencode(Operation *op)    { return NULL; }

    // none of these are mandatory but can be overriden and will be called before traversing any Operations
    virtual bool registerBuilder(Builder *b)    { return true; }
    virtual bool registerContext(Context *c)    { return true; }
    virtual bool registerLiteral(Literal *lv)   { return true; }
    virtual bool registerScope(Scope *s)        { return true; }
    virtual bool registerSymbol(Symbol *sym)    { return true; }
    virtual bool registerType(const Type *type) { return true; }
    virtual bool registerValue(Value *value)    { return true; }

protected:
    CodeGenerator *_cg;

    SUBCLASS_KINDSERVICE_DECL(Extensible,CodeGeneratorForExtension);
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(CODEGENERATORFOREXTENSION_INCL)

