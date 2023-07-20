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
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

INIT_JBALLOC_REUSECAT(FunctionType, Type)

TypeKind FunctionType::TYPEKIND = KindService::NoKind;
bool FunctionType::kindRegistered = false;

FunctionType::FunctionType(MEM_LOCATION(a), Extension *ext, TypeDictionary *dict, const Type *returnType, int32_t numParms, const Type ** parmTypes)
    : Type(MEM_PASSLOC(a), getTypeClassKind(), ext, dict, typeName(returnType, numParms, parmTypes), 0)
    , _returnType(returnType)
    , _numParms(numParms)
    , _parmTypes(parmTypes) {

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

const TypeKind
FunctionType::getTypeClassKind() {
    if (!kindRegistered) {
        TYPEKIND = Type::kindService.assignKind(NoTypeType::getTypeClassKind(), "Function");
        kindRegistered = true;
    }
    return TYPEKIND;
}


String
FunctionType::typeName(const Type * returnType, int32_t numParms, const Type **parmTypes) {
    String s(String("t").append(String::to_string(returnType->id())).append(String(" <- (")));
    if (numParms > 0)
        s.append(String("0:t")).append(String::to_string(parmTypes[0]->id()));
    for (auto p = 1; p < numParms; p++) {
        const Type *type = parmTypes[p];
        s.append(String(" ")).append(String::to_string(p)).append(String(":t")).append(String::to_string(type->id()));
    }
    s.append(String(")"));
    return s;
}

String
FunctionType::to_string(bool useHeader) const {
    String s(Type::base_string(useHeader));
    s.append(String("functionType"));
    return s;
}

void
FunctionType::logValue(TextLogger &lgr, const void *p) const {
    // TODO
}

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
    int parmNum = 0;
    for (int32_t p=0;p < numParms;p++) {
        const Type *parmType = _parmTypes[p];
        TypeMapper *parmMapper = repl->mapperForType(parmType);
        assert(parmMapper != NULL);
        for (int i=0;i < parmMapper->size();i++)
            newParmTypes[parmNum++] = parmMapper->next();
    }

    assert(parmNum == numNewParms);

    Allocator *mem = repl->comp()->compiler()->mem();
    const FunctionType *newType = new (mem) FunctionType(MEM_LOC(mem) , _ext, repl->comp()->typedict(), newReturnType, numNewParms, newParmTypes);
    return newType;
}

} // namespace Func
} // namespace JitBuilder
} // namespace OMR
