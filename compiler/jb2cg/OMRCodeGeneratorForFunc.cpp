/******************************************************************************* * Copyright (c) 2021, 2022 IBM Corp. and others *
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

#include "jb2/Func/Func.hpp"
#include "jb2cg/OMRCodeGenerator.hpp"
#include "jb2cg/OMRCodeGeneratorForFunc.hpp"
#include "jb2cg/OMRIlGen.hpp"


namespace OMR {
namespace JB2 {
namespace omrgen {

INIT_JBALLOC_REUSECAT(OMRCodeGeneratorForFunc, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(OMRCodeGeneratorForFunc,"OMRCodeGeneratorForFunc",CodeGeneratorForFunc,Extensible);

OMRCodeGeneratorForFunc::OMRCodeGeneratorForFunc(Allocator *a, OMRCodeGenerator *omrcg, Func::FunctionExtension *fx)
    : CodeGeneratorForFunc(a, omrcg, fx)
    , _fx(fx)
    , INIT_CG_FUNC_VFT_FIELDS(a) {

    INIT_CG_FUNC_HANDLERS(OMRCodeGeneratorForFunc);

    setTraceEnabled(false);
}

OMRCodeGeneratorForFunc::~OMRCodeGeneratorForFunc() {
}

OMRCodeGenerator *
OMRCodeGeneratorForFunc::omrcg() const {
    return cg()->refine<OMRCodeGenerator>();
}

OMRIlGen *
OMRCodeGeneratorForFunc::ilgen() const {
    return omrcg()->ilgen();
}

bool
OMRCodeGeneratorForFunc::registerType(const Type *t) {
    assert(t->isExactKind<Func::FunctionType>());
    return ilgen()->registerFunctionType(t);
}

bool
OMRCodeGeneratorForFunc::registerSymbol(Symbol *sym) {
    if (sym->isExactKind<Func::LocalSymbol>()) {
        ilgen()->createLocalSymbol(sym);
        return true;
    } else if (sym->isExactKind<Func::ParameterSymbol>()) {
        Func::ParameterSymbol *paramSym = sym->refine<Func::ParameterSymbol>();
        ilgen()->createParameterSymbol(sym, paramSym->index());
        return true;
    } else {
        assert(sym->isKind<Func::FunctionSymbol>());
        Func::FunctionSymbol *funcSym = sym->refine<Func::FunctionSymbol>();
        const Func::FunctionType *ft = funcSym->functionType();
        ilgen()->createFunctionSymbol(sym,
                                      funcSym->name().c_str(),
                                      funcSym->fileName().c_str(),
                                      funcSym->lineNumber().c_str(),
                                      ft->numParms(),
                                      ft->parmTypes(),
                                      ft->returnType(),
                                      funcSym->entryPoint());
        return true;
    }
}

void
OMRCodeGeneratorForFunc::setupbody(Compilation *comp) {
}

void
OMRCodeGeneratorForFunc::genbody(Compilation *comp) {
    ilgen()->entryPoint(comp->scope<Scope>()->entryPoint<BuilderEntry>()->builder());
}

DEFINE_CG_FUNC_HANDLER_DISPATCH(OMRCodeGeneratorForFunc);

Builder *
OMRCodeGeneratorForFunc::gencodeCall(Operation *op) {
    assert(op->action() == _fx->aCall);
    Func::Op_Call *opCall = static_cast<Func::Op_Call *>(op);
    ilgen()->call(opCall->location(), opCall, true);
    return NULL;
}

Builder *
OMRCodeGeneratorForFunc::gencodeCallVoid(Operation *op) {
    assert(op->action() == _fx->aCallVoid);
    Func::Op_CallVoid *opCallVoid = static_cast<Func::Op_CallVoid *>(op);
    ilgen()->call(opCallVoid->location(), opCallVoid, true);
    return NULL;
}

Builder *
OMRCodeGeneratorForFunc::gencodeLoad(Operation *op) {
    assert(op->action() == _fx->aLoad);
    Func::Op_Load *opLoad = static_cast<Func::Op_Load *>(op);
    ilgen()->load(opLoad->location(), opLoad->result(), opLoad->symbol());
    return NULL;
}

Builder *
OMRCodeGeneratorForFunc::gencodeReturn(Operation *op) {
    assert(op->action() == _fx->aReturn);
    Func::Op_Return *opReturn = static_cast<Func::Op_Return *>(op);
    ilgen()->returnValue(opReturn->location(), opReturn->operand());
    return NULL;
}

Builder *
OMRCodeGeneratorForFunc::gencodeReturnVoid(Operation *op) {
    assert(op->action() == _fx->aReturnVoid);
    Func::Op_ReturnVoid *opReturnVoid = static_cast<Func::Op_ReturnVoid *>(op);
    ilgen()->returnNoValue(opReturnVoid->location());
    return NULL;
}

Builder *
OMRCodeGeneratorForFunc::gencodeStore(Operation *op) {
    assert(op->action() == _fx->aStore);
    Func::Op_Store *opStore = static_cast<Func::Op_Store *>(op);
    ilgen()->store(opStore->location(), opStore->symbol(), opStore->operand());
    //jbmb()->Store(opStore->location(), opStore->parent(), opStore->symbol(), opStore->operand());
    return NULL;
}

} // namespace omrgen
} // namespace JB2
} // namespace OMR
