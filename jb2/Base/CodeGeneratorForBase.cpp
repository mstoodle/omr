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

#include "JBCore.hpp"
#include "Base/CodeGeneratorForBase.hpp"
#include "Base/BaseTypes.hpp"


namespace OMR {
namespace JB2 {
namespace Base {

INIT_JBALLOC_REUSECAT(CodeGeneratorForBase, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(CodeGeneratorForBase,"CodeGeneratorForBase",CodeGeneratorForExtension,Extensible);

CodeGeneratorForBase::CodeGeneratorForBase(Allocator *a, CodeGenerator *cg, Base::BaseExtension *bx)
    : CodeGeneratorForExtension(a, cg, CLASSKIND(CodeGeneratorForBase,Extensible), bx, "CodeGeneratorForBase")
    INIT_CG_BASE_VFT_FIELDS(a) {

    INIT_CG_BASE_HANDLERS(CodeGeneratorForBase);

    setTraceEnabled(false);
}

CodeGeneratorForBase::~CodeGeneratorForBase() {
    for (auto it = _structFieldNameMap.begin();it != _structFieldNameMap.end(); it++) {
        FieldMapType & mapper = it->second;
        for (auto it2 = mapper.begin(); it2 != mapper.end(); it2++) {
            const String *s = it2->second;
            delete s;
        }
    }
}

BaseExtension *
CodeGeneratorForBase::bx() const {
    return ext()->refine<BaseExtension>();
}

MISSING_CG_OP_HANDLER(CodeGeneratorForBase,)

MISSING_CG_OP_HANDLER(CodeGeneratorForBase, Const)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, Add)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, And)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, ConvertTo)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, Div)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, EqualTo)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, Mul)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, NotEqualTo)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, Sub)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, ForLoopUp)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, Goto)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpEqual)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpEqualZero)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpGreaterThan)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpGreaterOrEqual)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpLessThan)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpLessOrEqual)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpNotEqual)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpNotEqualZero)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpUnsignedGreaterThan)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpUnsignedGreaterOrEqual)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpUnsignedLessThan)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfCmpUnsignedLessOrEqual)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IfThenElse)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, Switch)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, LoadAt)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, StoreAt)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, LoadField)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, StoreField)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, LoadFieldAt)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, StoreFieldAt)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, CreateLocalArray)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, CreateLocalStruct)
MISSING_CG_OP_HANDLER(CodeGeneratorForBase, IndexAt)

MISSING_CG_CONSTFORTYPE_HANDLER(CodeGeneratorForBase, Int8)
MISSING_CG_CONSTFORTYPE_HANDLER(CodeGeneratorForBase, Int16)
MISSING_CG_CONSTFORTYPE_HANDLER(CodeGeneratorForBase, Int32)
MISSING_CG_CONSTFORTYPE_HANDLER(CodeGeneratorForBase, Int64)
MISSING_CG_CONSTFORTYPE_HANDLER(CodeGeneratorForBase, Float32)
MISSING_CG_CONSTFORTYPE_HANDLER(CodeGeneratorForBase, Float64)
MISSING_CG_CONSTFORTYPE_HANDLER(CodeGeneratorForBase, Address)

MISSING_CG_TYPE_REGISTRATION(CodeGeneratorForBase, Int8)
MISSING_CG_TYPE_REGISTRATION(CodeGeneratorForBase, Int16)
MISSING_CG_TYPE_REGISTRATION(CodeGeneratorForBase, Int32)
MISSING_CG_TYPE_REGISTRATION(CodeGeneratorForBase, Int64)
MISSING_CG_TYPE_REGISTRATION(CodeGeneratorForBase, Float32)
MISSING_CG_TYPE_REGISTRATION(CodeGeneratorForBase, Float64)
MISSING_CG_TYPE_REGISTRATION(CodeGeneratorForBase, Address)

const String &
CodeGeneratorForBase::registerFieldString(const Base::StructType * baseStructType, const Base::FieldType *fType, const String & name) {
    FieldMapType & mapper = _structFieldNameMap[baseStructType];
    auto it = mapper.find(fType);
    if (it != mapper.end())
        return *(it->second);

    String * s = new (allocator()) String(allocator(), allocator(), name);
    mapper.insert({fType, s}); // ownership passes to _structFieldNameMap
    return *s;
}

const String &
CodeGeneratorForBase::lookupFieldString(const Base::StructType * baseStructType, const Base::FieldType *fType) {
    const String & name = *_structFieldNameMap[baseStructType][fType];
    return name;
}

void
CodeGeneratorForBase::registerAllStructFields(const Base::StructType *sType, const Base::StructType * baseStructType, const String & fNamePrefix, size_t baseOffset) {
    for (auto fIt = sType->FieldsBegin(); fIt != sType->FieldsEnd(); fIt++) {
        const Base::FieldType *fType = fIt->second;
        String fieldName = fNamePrefix;
        fieldName.append(fType->name());
        size_t fieldOffset = baseOffset + fType->offset();

        const String & name = registerFieldString(baseStructType, fType, fieldName);
        if (fType->isKind<const Base::StructType>()) {
            // define a "dummy" field corresponding to the struct field itself, so we can ask for its address easily
            // in case this field's struct needs to be passed to anything
            registerField(fType, baseStructType->name(), name, compiler()->coreExt()->NoType(baseStructType->ir()), fieldOffset);
            const Base::StructType *innerStructType = fType->type()->refine<const Base::StructType>();
            String newPrefix = fieldName;
            fieldName.append(".");
            registerAllStructFields(innerStructType, baseStructType, newPrefix, fieldOffset);
        }
        else {
            registerField(fType, baseStructType->name(), name, fType->type(), fieldOffset);
        }
    }
}

void
CodeGeneratorForBase::missingCodeGeneratorTypeRegistration(LOCATION, const Type *type) {
    CompilationException e(PASSLOC, compiler(), bx()->CompileFail_CodeGeneratorMissingTypeRegistration);
    Allocator *mem = compiler()->mem();
    e.setMessageLine(String(mem, "Extension lacks a handler to register a specific Base Type"))
     .appendMessageLine(String(mem, "   Extension ").append(ext()->name()))
     .appendMessageLine(String(mem, "   CodeGenerator ").append(cg()->name()))
     .appendMessageLine(String(mem, "   for Type ").append(type->name()))
     .appendMessageLine(String(mem, "The code generator tried could not find a handler to register a specific Base type."))
     .appendMessageLine(String(mem, "Usually means that <CodeGenerator name>CodeGeneratorFor<Extension name>::regtype<Type>() does not know exist for that Type."));
    throw e;

}

void
CodeGeneratorForBase::missingCodeGeneratorConstForTypeHandler(LOCATION, Location *loc, Builder *parent, Value *result, Literal *lv) {
    CompilationException e(PASSLOC, compiler(), bx()->CompileFail_CodeGeneratorMissingConstForTypeHandler);
    Allocator *mem = compiler()->mem();
    e.setMessageLine(String(mem, "Extension lacks a handler to generate Const operations for a particular Type"))
     .appendMessageLine(String(mem, "   Extension ").append(ext()->name()))
     .appendMessageLine(String(mem, "   CodeGenerator ").append(cg()->name()))
     .appendMessageLine(String(mem, "   in Builder ").append(parent->name()))
     .appendMessageLine(String(mem, "   to Value v").append(String::to_string(mem, result->id())))
     .appendMessageLine(String(mem, "   for Literal ").append(String::to_string(mem, lv->id())))
     .appendMessageLine(String(mem, "The code generator tried could not find a handler to generate a constant load Operation for the type of the given literal."))
     .appendMessageLine(String(mem, "Usually means that <CodeGenerator name>CodeGeneratorFor<Extension name>::genconst<Type>() does not know exist for the literal's Type."));
    throw e;
}

} // namespace JB
} // namespace JB2
} // namespace OMR
