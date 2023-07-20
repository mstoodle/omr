/*******************************************************************************
 * Copyright (c) 2021, 2023 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distr/bution and is available at http://eclipse.org/legal/epl-2.0
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

#ifndef FUNCJBCODEGENERATOR_INCL
#define FUNCJBCODEGENERATOR_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "jb/JBCodeGenerator.hpp"

namespace OMR {
namespace JitBuilder {
namespace JB {

class JBMethodBuilder;

class FuncJBCodeGenerator : public JBCodeGenerator {
    JBALLOC_(FuncJBCodeGenerator)

public:
    DYNAMIC_ALLOC_ONLY(FuncJBCodeGenerator, Func::FunctionExtension *func);

    virtual void setupbody(JBMethodBuilder *jbmb, Compilation *comp);
    virtual void genbody(JBMethodBuilder *jbmb, Compilation *comp);
    virtual void gencode(JBMethodBuilder *jbmb, Operation *op);

    virtual bool registerBuilder(JBMethodBuilder *jbmb, Symbol *sym) { return false; }
    virtual bool registerSymbol(JBMethodBuilder *jbmb, Symbol *sym) { return false; }
    virtual bool registerType(JBMethodBuilder *jbmb, const Type *type);

protected:
    virtual void visitPreCompilation(Compilation * comp) { }

    typedef void (FuncJBCodeGenerator::*gencodeFunction)(JBMethodBuilder *jbmb, Operation *op);
    void gencodeCall(JBMethodBuilder *jbmb, Operation *op);
    void gencodeCallVoid(JBMethodBuilder *jbmb, Operation *op);
    void gencodeLoad(JBMethodBuilder *jbmb, Operation *op);
    void gencodeReturn(JBMethodBuilder *jbmb, Operation *op);
    void gencodeReturnVoid(JBMethodBuilder *jbmb, Operation *op);
    void gencodeStore(JBMethodBuilder *jbmb, Operation *op);

    #if 0 // Const should move to core extension
    void gencodeConst(JBMethodBuilder *jbmb, Operation *op);
    #endif

    Func::FunctionExtension *_fx;
    Array<gencodeFunction> _gencodeVFT;

    SUBCLASS_KINDSERVICE_DECL(Extensible,FuncJBCodeGenerator);
};

} // namespace JB
} // namespace JitBuilder
} // namespace OMR

#endif // defined(FUNCJBCODEGENERATOR_INCL)
