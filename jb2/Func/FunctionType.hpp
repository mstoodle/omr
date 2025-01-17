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

#ifndef FUNCTIONTYPES_INCL
#define FUNCTIONTYPES_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class FunctionExtension;
class FunctionType;
class FunctionTypeBuilder;

typedef void (FunctionTypeHelper)(FunctionType *funcType, FunctionTypeBuilder *builder);

class FunctionTypeBuilder : public Allocatable {
    JBALLOC_(FunctionTypeBuilder)

    friend class FunctionType;

public:
    ALL_ALLOC_ALLOWED(FunctionTypeBuilder, Compilation *comp);
    ALL_ALLOC_ALLOWED(FunctionTypeBuilder, IR *ir);

    FunctionTypeBuilder & setHelper(FunctionTypeHelper *helper) { _helper = helper; return *this; }
    FunctionTypeBuilder & setReturnType(const Type * type) { _returnType = type; return *this; }
    FunctionTypeBuilder & addParameterType(const Type * type) { _parameterTypes.push_back(type); return *this; }

    const FunctionType * create(FunctionExtension *fx, Compilation *comp); 
    const FunctionType * create(MEM_LOCATION(a), FunctionExtension *fx, IR *ir); 

protected:
    IR *ir() const { return _ir; }
    const Type * returnType() const { return _returnType; };
    size_t numParameters() const { return _parameterTypes.length(); }
    List<const Type *>::Iterator parameterTypes() const { return _parameterTypes.iterator(); }

    IR *_ir;
    FunctionTypeHelper * _helper;
    const Type * _returnType;
    List<const Type *> _parameterTypes;
};

DECL_TYPE_CLASS_WITH_STATE(FunctionType,Type,FunctionExtension,
    friend class TypeDictionary;
    const Type * _returnType;
    int32_t _numParms;
    const Type ** _parmTypes;

    FunctionExtension *funcExt();
    FunctionExtension *funcExt() const;

public:
    DYNAMIC_ALLOC_ONLY(FunctionType, LOCATION, FunctionExtension *fx, FunctionTypeBuilder &ftb);

    //virtual Literal *literal(LOCATION, IR *ir, void * functionValue);
    virtual String to_string(Allocator *mem, bool useHeader=false) const;

    const Type *returnType() const { return _returnType; }
    int32_t numParms() const { return _numParms; }
    const Type *parmType(int p) const { return _parmTypes[p]; }
    const Type **parmTypes() const { return _parmTypes; }

    //virtual const Type * replace(TypeReplacer *repl);

    static String typeName(Allocator *mem, FunctionTypeBuilder & ftb);
)

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // !defined(FUNCTIONTYPES_INCL)
