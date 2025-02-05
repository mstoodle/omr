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

#ifndef MEMORYOPERATIONS_INCL
#define MEMORYOPERATIONS_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

DECL_OPERATION_CLASS(Op_LoadAt, OperationR1V1, BaseExtension,
    Op_LoadAt(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aLoadAt, Value *result, Value *value);
)

DECL_OPERATION_CLASS(Op_StoreAt, OperationR0V2, BaseExtension,
    Op_StoreAt(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aStoreAt, Value *address, Value *value);
)

DECL_OPERATION_CLASS(Op_LoadField, OperationR1T1V1, BaseExtension,
    Op_LoadField(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aLoadField, Value *result, const FieldType *fieldType, Value *structValue);
)

DECL_OPERATION_CLASS(Op_StoreField, OperationR0T1V2, BaseExtension,
    Op_StoreField(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aStoreField, const FieldType *fieldType, Value *structValue, Value *value);
)

DECL_OPERATION_CLASS(Op_LoadFieldAt, OperationR1T1V1, BaseExtension,
    Op_LoadFieldAt(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aLoadFieldAt, Value *result, const FieldType *fieldType, Value *pStruct);
)

DECL_OPERATION_CLASS(Op_StoreFieldAt, OperationR0T1V2, BaseExtension,
    Op_StoreFieldAt(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aStoreFieldAt, const FieldType *fieldType, Value *pStruct, Value *value);
)

DECL_OPERATION_CLASS(Op_CreateLocalArray, OperationR1L1T1, BaseExtension,
protected:
    Op_CreateLocalArray(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCreateLocalArray, Value *result, Literal *numElements, const PointerType *pElementType);
)

DECL_OPERATION_CLASS(Op_CreateLocalStruct, OperationR1T1, BaseExtension,
    Op_CreateLocalStruct(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aCreateLocalStruct, Value *result, const StructType *structType);
)

DECL_OPERATION_CLASS(Op_IndexAt, OperationR1V2, BaseExtension,
    Op_IndexAt(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aIndexAt, Value *result, Value *base, Value *index);
)

#if 0
class LoadField : public OperationR1V1T1
    {
    public:
    static LoadField * create(Builder * parent, Value * result, FieldType *fieldType, Value *structBase)
        { return new LoadField(parent, result, fieldType, structBase); }

    FieldType *getFieldType() const { return static_cast<FieldType *>(_type); }

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(results);
        return create(b, results[0], getFieldType(), operand(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(results && operands && NULL == builders);
        return create(b, results[0], getFieldType(), operands[0]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    LoadField(Builder * parent, Value * result, FieldType *fieldType, Value * structBase)
        : OperationR1V1T1(aLoadField, parent, result, fieldType, structBase)
        { }
    };

class LoadIndirect : public OperationR1V1T1
    {
    public:
    static LoadIndirect * create(Builder * parent, Value * result, FieldType *fieldType, Value *structBase)
        { return new LoadIndirect(parent, result, fieldType, structBase); }

    FieldType *getFieldType() const { return static_cast<FieldType *>(_type); }

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(results);
        return create(b, results[0], getFieldType(), operand(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(results && operands && NULL == builders);
        return create(b, results[0], getFieldType(), operands[0]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    LoadIndirect(Builder * parent, Value * result, FieldType *fieldType, Value * structBase)
        : OperationR1V1T1(aLoadIndirect, parent, result, fieldType, structBase)
        { }
    };

class StoreField : public OperationR0T1V2
    {
    public:
    static StoreField * create(Builder * parent, FieldType *fieldType, Value *structBase, Value *value)
        { return new StoreField(parent, fieldType, structBase, value); }

    FieldType *getFieldType() const { return static_cast<FieldType *>(_type); }

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, getFieldType(), operand(0), operand(1));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && NULL == builders);
        return create(b, getFieldType(), operands[0], operands[1]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    StoreField(Builder * parent, FieldType *fieldType, Value * structBase, Value *value)
        : OperationR0T1V2(aStoreField, parent, fieldType, structBase, value)
        { }
    }; 

class StoreIndirect : public OperationR0T1V2
    {
    public:
    static StoreIndirect * create(Builder * parent, FieldType *fieldType, Value *structBase, Value *value)
        { return new StoreIndirect(parent, fieldType, structBase, value); }

    FieldType *getFieldType() const { return static_cast<FieldType *>(_type); }

    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(NULL == results);
        return create(b, getFieldType(), operand(0), operand(1));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(NULL == results && operands && NULL == builders);
        return create(b, getFieldType(), operands[0], operands[1]);
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    StoreIndirect(Builder * parent, FieldType *fieldType, Value * structBase, Value *value)
        : OperationR0T1V2(aStoreIndirect, parent, fieldType, structBase, value)
        { }
    };

class CreateLocalArray : public OperationR1L1T1
    {
    public:
    static CreateLocalArray * create(Builder * parent, Value * result, int32_t numElements, Type * elementType)
        { return new CreateLocalArray(parent, result, numElements, elementType); }
    
    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(results);
        return create(b, results[0], literal(0)->getInt32(), type(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(results && NULL == operands && NULL == builders);
        return create(b, results[0], literal(0)->getInt32(), type(0));
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    CreateLocalArray(Builder * parent, Value * result, int32_t numElements, Type * elementType);
    };

class CreateLocalStruct : public OperationR1T1
    {
    public:
    static CreateLocalStruct * create(Builder * parent, Value * result, Type * structType)
        { return new CreateLocalStruct(parent, result, structType); }
    
    virtual Operation * clone(Builder *b, Value **results) const
        {
        assert(results);
        return create(b, results[0], type(0));
        }
    virtual Operation * clone(Builder *b, Value **results, Value **operands, Builder **builders) const
        {
        assert(results && NULL == operands && NULL == builders);
        return create(b, results[0], type(0));
        }
    virtual void cloneTo(Builder *b, ValueMapper **resultMappers, ValueMapper **operandMappers, TypeMapper **typeMappers, LiteralMapper **literalMapppers, SymbolMapper **symbolMappers, BuilderMapper **builderMappers) const;

    virtual Operation * clone(Builder *b, OperationCloner *cloner) const;

    protected:
    CreateLocalStruct(Builder * parent, Value * result, Type * structType);
    };

#endif

} // namespace Base
} // namespace JB2
} // namespace OMR

#endif // !defined(MEMORYOPERATIONS_INCL)

