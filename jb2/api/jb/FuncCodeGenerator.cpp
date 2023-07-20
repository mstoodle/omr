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

#include "Func/Func.hpp"
#include "jb/FuncCodeGenerator.hpp"
#include "jb/JBMethodBuilder.hpp"


namespace OMR {
namespace JitBuilder {
namespace JB {

INIT_JBALLOC_REUSECAT(FuncJBCodeGenerator, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(FuncJBCodeGenerator,"FuncJBCodeGenerator",JBCodeGenerator,Extensible);

FuncJBCodeGenerator::FuncJBCodeGenerator(Allocator *a, Func::FunctionExtension *fx)
    : JBCodeGenerator(a, fx)
    , _fx(fx)
    , _gencodeVFT(NULL, a) {

    _gencodeVFT.assign(fx->aCall, &FuncJBCodeGenerator::gencodeCall);
    _gencodeVFT.assign(fx->aCallVoid, &FuncJBCodeGenerator::gencodeCallVoid);
    _gencodeVFT.assign(fx->aLoad, &FuncJBCodeGenerator::gencodeLoad);
    _gencodeVFT.assign(fx->aReturn, &FuncJBCodeGenerator::gencodeReturn);
    _gencodeVFT.assign(fx->aReturnVoid, &FuncJBCodeGenerator::gencodeReturnVoid);
    _gencodeVFT.assign(fx->aStore, &FuncJBCodeGenerator::gencodeStore);

    setTraceEnabled(false);
}

FuncJBCodeGenerator::~FuncJBCodeGenerator() {
}

bool
FuncJBCodeGenerator::registerType(JBMethodBuilder *jbmb, const Type *t) {
    return true; // FunctionTYpe handled by Address which will be registered independently
}

void
FuncJBCodeGenerator::setupbody(JBMethodBuilder *jbmb, Compilation *comp) {
    Func::FunctionCompilation *fcomp = static_cast<Func::FunctionCompilation *>(comp);

    jbmb->FunctionName(fcomp->func()->name());
    jbmb->FunctionFile(fcomp->func()->fileName());
    jbmb->FunctionLine(fcomp->func()->lineNumber());

    Func::FunctionContext *fcontext = fcomp->context<Func::FunctionContext>();

    jbmb->FunctionReturnType(fcontext->returnType());

    for (auto paramIt = fcontext->parameters();paramIt.hasItem(); paramIt++) {
        const Func::ParameterSymbol *parameter = paramIt.item();
        jbmb->Parameter(parameter->name(), parameter->type());
    }
    for (auto localIt = fcontext->locals();localIt.hasItem();localIt++) {
        const Func::LocalSymbol *symbol = localIt.item();
        jbmb->Local(symbol->name(), symbol->type());
    }
    for (auto fnIt = fcontext->functions();fnIt.hasItem();fnIt++) {
        const Func::FunctionSymbol *fSym = fnIt.item();
        const Func::FunctionType *fType = fSym->functionType();
        jbmb->DefineFunction(fSym->name(),
                             fSym->fileName(),
                             fSym->lineNumber(),
                             fSym->entryPoint(),
                             fType->returnType(),
                             fType->numParms(),
                             fType->parmTypes());
    }
}

void
FuncJBCodeGenerator::genbody(JBMethodBuilder *jbmb, Compilation *comp) {
    jbmb->EntryPoint(comp->scope<Scope>()->entryPoint<BuilderEntry>()->builder());
}

void
FuncJBCodeGenerator::gencode(JBMethodBuilder *jbmb, Operation *op) {
    ActionID a = op->action();
    gencodeFunction f = _gencodeVFT[a];
    (this->*f)(jbmb, op);
}

void
FuncJBCodeGenerator::gencodeCall(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _fx->aCall);
    Func::Op_Call *opCall = static_cast<Func::Op_Call *>(op);
    Func::FunctionSymbol *funcSym = opCall->symbol()->refine<Func::FunctionSymbol>();
    const Func::FunctionType *funcType = funcSym->functionType();
    jbmb->Call(opCall->location(), opCall->parent(), opCall->result(), funcSym->name(), opCall->numOperands(), opCall->operands());
}

void
FuncJBCodeGenerator::gencodeCallVoid(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _fx->aCallVoid);
    Func::Op_CallVoid *opCallVoid = static_cast<Func::Op_CallVoid *>(op);
    Func::FunctionSymbol *funcSym = opCallVoid->symbol()->refine<Func::FunctionSymbol>();
    const Func::FunctionType *funcType = funcSym->functionType();
    jbmb->Call(opCallVoid->location(), opCallVoid->parent(), funcSym->name(), opCallVoid->numOperands(), opCallVoid->operands());
}

void
FuncJBCodeGenerator::gencodeLoad(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _fx->aLoad);
    Func::Op_Load *opLoad = static_cast<Func::Op_Load *>(op);
    jbmb->Load(opLoad->location(), opLoad->parent(), opLoad->result(), opLoad->symbol());
}

void
FuncJBCodeGenerator::gencodeReturn(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _fx->aReturn);
    Func::Op_Return *opReturn = static_cast<Func::Op_Return *>(op);
    jbmb->Return(opReturn->location(), opReturn->parent(), opReturn->operand());
}

void
FuncJBCodeGenerator::gencodeReturnVoid(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _fx->aReturnVoid);
    Func::Op_ReturnVoid *opReturnVoid = static_cast<Func::Op_ReturnVoid *>(op);
    jbmb->Return(opReturnVoid->location(), opReturnVoid->parent());
}

void
FuncJBCodeGenerator::gencodeStore(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _fx->aStore);
    Func::Op_Store *opStore = static_cast<Func::Op_Store *>(op);
    jbmb->Store(opStore->location(), opStore->parent(), opStore->symbol(), opStore->operand());
}


#if 0
// Const should move to core
void
FuncJBCodeGenerator::gencodeConst(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _fx->compiler()->coreExt()->aConst);
    assert(op->result()->type()->isKind<Func::FunctionType>());
    jbmb->ConstAddress(LOC, op->builder(), op->result(), op->literal()->value<void * const>());
}
#endif

} // namespace JB
} // namespace JitBuilder
} // namespace OMR
