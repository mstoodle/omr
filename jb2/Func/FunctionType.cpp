/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.acompache.org/licenses/LICENSE-2.0.
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

#include "JBCore.hpp"
#include "Func/Function.hpp"
#include "Func/FunctionCompilation.hpp"
#include "Func/FunctionExtension.hpp"
#include "Func/FunctionIRAddon.hpp"
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

INIT_JBALLOC_REUSECAT(FunctionTypeBuilder, Type)

FunctionTypeBuilder::FunctionTypeBuilder(Compilation *comp)
    : _ir(comp->ir())
    , _helper(NULL)
    , _returnType(NULL)
    , _parameterTypes(NULL, comp->ir()->mem()) {

}

FunctionTypeBuilder::FunctionTypeBuilder(Allocator *a, Compilation *comp)
    : _ir(comp->ir())
    , _helper(NULL)
    , _returnType(NULL)
    , _parameterTypes(NULL, a) {

}

FunctionTypeBuilder::FunctionTypeBuilder(IR *ir)
    : _ir(ir)
    , _helper(NULL)
    , _returnType(NULL)
    , _parameterTypes(NULL, ir->mem()) {

}

FunctionTypeBuilder::FunctionTypeBuilder(Allocator *a, IR *ir)
    : _ir(ir)
    , _helper(NULL)
    , _returnType(NULL)
    , _parameterTypes(NULL, a) {

}

FunctionTypeBuilder::~FunctionTypeBuilder() {

}

const FunctionType *
FunctionTypeBuilder::create(FunctionExtension *fx, Compilation *comp) {
    Allocator *mem = comp->ir()->mem();
    return new (mem) FunctionType(MEM_LOC(mem), fx, *this);
}

const FunctionType *
FunctionTypeBuilder::create(MEM_LOCATION(a), FunctionExtension *fx, IR *ir) {
    return new (a) FunctionType(MEM_PASSLOC(a), fx, *this);
}

DEFINE_TYPE_CLASS_COMMON(FunctionType, Type, "FunctionType",)

FunctionType::FunctionType(MEM_LOCATION(a), FunctionExtension *fx, FunctionTypeBuilder &ftb)
    : Type(MEM_PASSLOC(a), CLASSKIND(FunctionType, Extensible), fx, ftb.ir(), typeName(a, ftb))
    , _returnType(ftb.returnType())
    , _numParms(ftb.numParameters())
    , _parmTypes((_numParms > 0) ? a->allocate<const Type *>(_numParms) : NULL) {

    int p=0;
    for (auto it = ftb.parameterTypes(); it.hasItem(); it++) {
        const Type *parmType = it.item();
        _parmTypes[p++] = parmType;
    }
    
    ftb.ir()->addon<FunctionIRAddon>()->registerFunctionType(this);
}

FunctionType::FunctionType(Allocator *a, const FunctionType *source, IRCloner *cloner)
    : Type(a, source, cloner)
    , _returnType(cloner->clonedType(source->_returnType))
    , _numParms(source->_numParms)
    , _parmTypes(cloner->clonedTypeArray(source->_numParms, source->_parmTypes)) {
}

FunctionType::~FunctionType() {
    delete[] _parmTypes;
}

FunctionExtension *
FunctionType::funcExt() {
    return static_cast<FunctionExtension *>(_ext);
}

FunctionExtension *
FunctionType::funcExt() const {
    return static_cast<FunctionExtension * const>(_ext);
}
 
String
FunctionType::typeName(Allocator *mem, FunctionTypeBuilder & ftb) {
    String s(mem, String(mem, "t").append(String::to_string(mem, ftb.returnType()->id())).append(String(mem, " <- (")));
    auto it = ftb.parameterTypes();
    if (it.hasItem()) {
        const Type *type = it.item();
        s.append(String(mem, "t")).append(String::to_string(mem, type->id()));
        it++;
    }
    for (int p=1 ;it.hasItem(); p++, it++) {
        const Type *type = it.item();
        s.append(String(mem, ",")).append(String(mem, "t")).append(String::to_string(mem, type->id()));
        //s.append(String(mem, " ")).append(String::to_string(mem, p)).append(String(mem, ":t")).append(String::to_string(mem, type->id()));
    }
    s.append(String(mem, ")"));
    return s;
}

String
FunctionType::to_string(Allocator *mem, bool useHeader) const {
    String s(Type::base_string(mem, useHeader));
    s.append(String(mem, "functionType"));
    return s;
}

void
FunctionType::logValue(TextLogger &lgr, const void *p) const {
    // TODO
}

bool
FunctionType::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return false;
}

void
FunctionType::logLiteral(TextLogger & lgr, const Literal *lv) const {

}

#if 0
const Type *
FunctionType::replace(TypeReplacer *repl) {
    const Type *returnType = _returnType;
    assert(!repl->isExploded(returnType)); // can't explode return types yet
    const Type *newReturnType = repl->singleMappedType(returnType);

    // count how many parameters are needed after exploding types
    int32_t numParms = _numParms;
    int32_t numNewParms = 0; // each original parameter explodes to at least one remapped parameter
    for (int32_t p=0;p < numParms;p++) {
        const Type *parmType = _parmTypes[p];
        TypeMapper *parmMapper = repl->mapperForType(parmType);
        assert(parmMapper != NULL);
        numNewParms += parmMapper->size();
    }

    // allocate array and then assign the mapped parameter types
    const Type **newParmTypes = new const Type *[numNewParms];
    uint32_t parmNum = 0;
    for (uint32_t p=0;p < numParms;p++) {
        const Type *parmType = _parmTypes[p];
        TypeMapper *parmMapper = repl->mapperForType(parmType);
        assert(parmMapper != NULL);
        for (uint32_t i=0;i < parmMapper->size();i++)
            newParmTypes[parmNum++] = parmMapper->next();
    }

    assert(parmNum == numNewParms);

    FunctionTypeBuilder ftb(repl->comp()->ir());
    ftb.setReturnType(newReturnType);
    for (uint32_t p=0;p < numParms;p++)
        ftb.addParameterType(newParmTypes[p]);

    Allocator *mem = repl->comp()->compiler()->mem();
    const FunctionType *newType = ftb.create(MEM_LOC(mem), funcExt(), repl->comp()->ir());
    return newType;
}
#endif

} // namespace Func
} // namespace JB2
} // namespace OMR
