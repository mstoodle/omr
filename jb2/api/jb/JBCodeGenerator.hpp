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

#ifndef JBCODEGENERATOR_INCL
#define JBCODEGENERATOR_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JitBuilder {
namespace JB {

class JBMethodBuilder;

typedef void *TRType;

class JBCodeGenerator : public CodeGenerator {
    JBALLOC_(JBCodeGenerator)

public:
    DYNAMIC_ALLOC_ONLY(JBCodeGenerator, Extension *ext);

    void * entryPoint() const  { return _entryPoint; }
    int32_t returnCode() const { return _compileReturnCode; }
    JBMethodBuilder *jbmb() const { return _jbmb; }

    virtual CompilerReturnCode perform(Compilation *comp);
    virtual Builder *gencode(CodeGenerator *cgPrime, Operation *op);

    virtual void setupbody(JBMethodBuilder *jbmb, Compilation *comp) { }
    virtual void createbuilder(JBMethodBuilder *jbmb, Builder *b) { }

    virtual void genbody(JBMethodBuilder *jbmb, Compilation *comp) { }
    virtual void gencode(JBMethodBuilder *jbmb, Operation *op);

    virtual void connectsuccessors(JBMethodBuilder *jbmb, Builder *b) { }

    virtual bool registerBuilder(JBMethodBuilder *jbmb, Builder *b)    { return true; }
    virtual bool registerContext(JBMethodBuilder *jbmb, Context *c)    { return true; }
    virtual bool registerLiteral(JBMethodBuilder *jbmb, Literal *lv)   { return true; }
    virtual bool registerScope(JBMethodBuilder *jbmb, Scope *s)        { return true; }
    virtual bool registerSymbol(JBMethodBuilder *jbmb, Symbol *sym)    { return true; }
    virtual bool registerType(JBMethodBuilder *jbmb, const Type *type) { return true; }
    virtual bool registerValue(JBMethodBuilder *jbmb, Value *value)    { return true; }

protected:
    virtual void visitPreCompilation(Compilation * comp);
    virtual void visitBuilderPostOps(Builder * b);
    virtual void visitOperation(Operation * op);
    virtual void visitPostCompilation(Compilation *comp);

    JBMethodBuilder *_jbmb;
    void *  _entryPoint;
    int32_t _jbCompileReturnCode;
    CompilerReturnCode _compileReturnCode;

    SUBCLASS_KINDSERVICE_DECL(Extensible,JBCodeGenerator);
};

} // namespace JB
} // namespace JitBuilder
} // namespace OMR

#endif // defined(JBCODEGENERATOR_INCL)
