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

#ifndef JBMETHODBUILDER_INCL
#define JBMETHODBUILDER_INCL

#include <map>
#include "JBCore.hpp"

namespace TR { class BytecodeBuilder; }
namespace TR { class IlBuilder; }
namespace TR { class IlType; }
namespace TR { class IlValue; }
namespace TR { class MethodBuilder; }
namespace TR { class TypeDictionary; }

namespace OMR {
namespace JB2 {
namespace jbgen {

class FunctionBuilder;

typedef void *TRType;

class JBMethodBuilder : public Loggable {
    JBALLOC_(JBMethodBuilder)

    friend class JBCodeGenerator;
public:
    JBMethodBuilder(Compilation *comp);

    void setMethodBuilder(void *mb) { _mb = reinterpret_cast<TR::MethodBuilder *>(mb); }

    void * entryPoint() const  { return _entryPoint; }
    int32_t returnCode() const { return _compileReturnCode; }

    void registerTypes(TypeDictionary *dict);
    bool typeRegistered(const Type *t);

    void createBuilder(const Builder * b);
    void createBytecodeBuilder(const Builder * b, int32_t bcIndex, const String & name);
    void addFallThroughBuilder(const Builder * b, const Builder * ftb);
    void addSuccessorBuilder(const Builder * b, const Builder * sb);

    void registerNoType(const Type * t);
    void registerInt8(const Type * t);
    void registerInt16(const Type * t);
    void registerInt32(const Type * t);
    void registerInt64(const Type * t);
    void registerFloat(const Type * t);
    void registerDouble(const Type * t);
    void registerAddress(const Type * t);
    void registerBuilder(const Builder * b, TR::IlBuilder *omr_b = NULL);
    void registerBytecodeBuilder(const Builder * bcb, TR::BytecodeBuilder *omr_bcb = NULL);
    void registerPointer(const Type *ptrType, const Type *baseType);
    void registerStruct(const Type *type);
    void registerField(const String & structName, const String & fieldName, const Type *type, size_t offset);
    void closeStruct(const String & structName);

    void FunctionName(const String & name);
    void FunctionFile(const String & file);
    void FunctionLine(const String & line);
    void FunctionReturnType(const Type *type);
    void Parameter(const String & name, const Type * type);
    void Local(const String & name, const Type * type);
    void DefineFunction(const String & name,
                        const String & fileName,
                        const String & lineNumber,
                        void *entryPoint,
                        const Type * returnType,
                        int32_t numParms,
                        const Type **parmTypes);


    void ConstInt8(Location *loc, Builder *b, Value *result, const int8_t v);
    void ConstInt16(Location *loc, Builder *b, Value *result, const int16_t v);
    void ConstInt32(Location *loc, Builder *b, Value *result, const int32_t v);
    void ConstInt64(Location *loc, Builder *b, Value *result, const int64_t v);
    void ConstFloat(Location *loc, Builder *b, Value *result, const float f);
    void ConstDouble(Location *loc, Builder *b, Value *result, const double v);
    void ConstAddress(Location *loc, Builder *b, Value *result, const void *v);

    void Add(Location *loc, Builder *b, Value *result, Value *left, Value *right);
    void And(Location *loc, Builder *b, Value *result, Value *left, Value *right);
    void ConvertTo(Location *loc, Builder *b, Value *result, const Type *type, Value *value);
    void Div(Location *loc, Builder *b, Value *result, Value *left, Value *right);
    void EqualTo(Location *loc, Builder *b, Value *result, Value *left, Value *right);
    void Mul(Location *loc, Builder *b, Value *result, Value *left, Value *right);
    void NotEqualTo(Location *loc, Builder *b, Value *result, Value *left, Value *right);
    void Sub(Location *loc, Builder *b, Value *result, Value *left, Value *right);

    void AppendBuilder(Location *loc, Builder *parent, Builder *b);
    void Call(Location *loc, Builder *b, Value *result, const String & targetName, size_t numArgs, ValueIterator argIt);
    void Call(Location *loc, Builder *b, const String & targetName, size_t numArgs, ValueIterator argIt);
    void EntryPoint(Builder *entryBuilder);
    void ForLoopUp(Location *loc, Builder *b, Symbol *loopVariable, Value *initial, Value *final, Value *bump, Builder *loopBody, Builder *loopBreak, Builder *loopContinue);
    void Goto(Location *loc, Builder *b, Builder *target);
    void IfCmpEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpEqualZero(Location *loc, Builder *b, Builder *target, Value *value);
    void IfCmpGreaterThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpGreaterOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpLessThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpLessOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpNotEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpNotEqualZero(Location *loc, Builder *b, Builder *target, Value *value);
    void IfCmpUnsignedGreaterThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpUnsignedGreaterOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpUnsignedLessThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpUnsignedLessOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right);
    void IfThen(Location *loc, Builder *b, Builder *thenPath, Value *condition);
    void IfThenElse(Location *loc, Builder *b, Builder *thenPath, Builder *elsePath, Value *selector);
    void Return(Location *loc, Builder *b);
    void Return(Location *loc, Builder *b, Value *value);
    void Switch(Location *loc, Builder *b, Builder *defaultBuilder, Value *selector, int32_t numCases, Literal **lvs, Builder **caseBuilders, bool *fallThroughs);

    void Load(Location *loc, Builder *b, Value *result, Symbol *sym);
    void Store(Location *loc, Builder *b, Symbol *sym, Value *value);
    void LoadAt(Location *loc, Builder *b, Value *result, Value *ptrValue);
    void StoreAt(Location *loc, Builder *b, Value *ptrValue, Value *value);
    void LoadIndirect(Location *loc, Builder *b, Value *result, const String & structName, const String & fieldName, Value *pStruct);
    void StoreIndirect(Location *loc, Builder *b, const String & structName, const String & fieldName, Value *pStruct, Value *value);
    void StoreOver(Location *loc, Builder *b, Value *target, Value *source);
    void CreateLocalArray(Location *loc, Builder *b, Value *result, Literal *numElements, const Type *elementType);
    void CreateLocalStruct(Location *loc, Builder *b, Value *result, const Type * structType);
    void IndexAt(Location *loc, Builder *b, Value *result, Value *base, Value *index);

    void generateFunctionAPI(FunctionBuilder *fb);

protected:
#if 0
    virtual FunctionBuilder * transformFunctionBuilder(FunctionBuilder * fb);
    virtual Builder * transformOperation(Operation *op);
    virtual FunctionBuilder * transformFunctionBuilderAtEnd(FunctionBuilder * fb);
#endif

    const char * findOrCreateString(const String & str);
    void registerValue(const Value * v, TR::IlValue *omr_v);

    TR::IlBuilder *map(const Builder * b, bool checkNull=true);
    TR::BytecodeBuilder *mapBytecodeBuilder(const Builder * b, bool checkNull=true);
    TR::IlValue *map(const Value * v);
    TR::IlType *map(const Type * t);

#if 0
    TR::IlType *registerStructFields(TR::TypeDictionary * types, StructType * sType, char * structName, String fNamePrefix, size_t baseOffset);
    void *registerCase(TR::IlBuilder *omr_b, Case * c); // void * so we don't have to include IlBuilder.hpp in this header
#endif

    void printAllMaps();

    std::map<BuilderID,TR::IlBuilder *> _builders;
    std::map<BuilderID,TR::BytecodeBuilder *> _bytecodeBuilders;
    //std::map<CaseID,void *> _cases; // void * so we don't need to include IlBuilder.hpp in this header
    std::map<TypeID,TR::IlType *> _types;
    std::map<ValueID,TR::IlValue *> _values;
    //std::map<FunctionID,TR::MethodBuilder *> _methodBuilders;
    //std::map<TypeDictionaryID,TR::TypeDictionary *> _typeDictionaries;
    std::map<const char *,const char *> _strings;

    Compilation *_comp;
    TR::MethodBuilder *_mb;
    void *  _entryPoint;
    int32_t _compileReturnCode;
};

} // namespace jbgen
} // namespace JB2
} // namespace OMR

#endif // defined(JBMETHODBUILDER_INCL)
