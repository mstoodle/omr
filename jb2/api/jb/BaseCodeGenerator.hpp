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

#ifndef BASEJBCODEGENERATOR_INCL
#define BASEJBCODEGENERATOR_INCL

#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "jb/JBCodeGenerator.hpp"

namespace OMR {
namespace JitBuilder {
namespace JB {

class JBMethodBuilder;

class BaseJBCodeGenerator : public JBCodeGenerator {
    JBALLOC_(BaseJBCodeGenerator)

public:
    DYNAMIC_ALLOC_ONLY(BaseJBCodeGenerator, Base::BaseExtension *base);

    virtual void gencode(JBMethodBuilder *jbmb, Operation *op);

    virtual bool registerSymbol(JBMethodBuilder *jbmb, Symbol *sym) { return false; }
    virtual bool registerType(JBMethodBuilder *jbmb, const Type *type);

protected:
    virtual void visitPreCompilation(Compilation * comp) { }

    void registerAllStructFields(JBMethodBuilder *jbmb, const Base::StructType *sType, String structName, String fNamePrefix, size_t baseOffset) const;

    typedef void (BaseJBCodeGenerator::*gencodeFunction)(JBMethodBuilder *jbmb, Operation *op);
    void gencodeConst(JBMethodBuilder *jbmb, Operation *op);
    void gencodeAdd(JBMethodBuilder *jbmb, Operation *op);
    void gencodeConvertTo(JBMethodBuilder *jbmb, Operation *op);
    void gencodeMul(JBMethodBuilder *jbmb, Operation *op);
    void gencodeSub(JBMethodBuilder *jbmb, Operation *op);
    void gencodeForLoopUp(JBMethodBuilder *jbmb, Operation *op);
    void gencodeGoto(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpEqual(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpEqualZero(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpGreaterThan(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpGreaterOrEqual(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpLessThan(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpLessOrEqual(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpNotEqual(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpNotEqualZero(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpUnsignedGreaterThan(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpUnsignedGreaterOrEqual(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpUnsignedLessThan(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIfCmpUnsignedLessOrEqual(JBMethodBuilder *jbmb, Operation *op);
    void gencodeLoadAt(JBMethodBuilder *jbmb, Operation *op);
    void gencodeStoreAt(JBMethodBuilder *jbmb, Operation *op);
    void gencodeLoadField(JBMethodBuilder *jbmb, Operation *op);
    void gencodeStoreField(JBMethodBuilder *jbmb, Operation *op);
    void gencodeLoadFieldAt(JBMethodBuilder *jbmb, Operation *op);
    void gencodeStoreFieldAt(JBMethodBuilder *jbmb, Operation *op);
    void gencodeCreateLocalArray(JBMethodBuilder *jbmb, Operation *op);
    void gencodeCreateLocalStruct(JBMethodBuilder *jbmb, Operation *op);
    void gencodeIndexAt(JBMethodBuilder *jbmb, Operation *op);

    typedef void (BaseJBCodeGenerator::*genconstFunction)(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstInt8(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstInt16(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstInt32(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstInt64(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstFloat32(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstFloat64(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstAddress(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);
    void genconstStruct(JBMethodBuilder *jbmb, Location *loc, Builder *parent, Value *result, Literal *lv);

    typedef void (BaseJBCodeGenerator::*regtypeFunction)(JBMethodBuilder *jbmb, const Type *t);
    void regtypeInt8(JBMethodBuilder *jbmb, const Type *Int8);
    void regtypeInt16(JBMethodBuilder *jbmb, const Type *Int16);
    void regtypeInt32(JBMethodBuilder *jbmb, const Type *Int32);
    void regtypeInt64(JBMethodBuilder *jbmb, const Type *Int32);
    void regtypeFloat32(JBMethodBuilder *jbmb, const Type *Float32);
    void regtypeFloat64(JBMethodBuilder *jbmb, const Type *Float64);
    void regtypeAddress(JBMethodBuilder *jbmb, const Type *Address);

    Base::BaseExtension *_base;
    Array<gencodeFunction> _gencodeVFT;
    Array<genconstFunction> _genconstVFT;
    Array<regtypeFunction> _regtypeVFT;

    SUBCLASS_KINDSERVICE_DECL(Extensible,BaseJBCodeGenerator);
};

} // namespace JB
} // namespace JitBuilder
} // namespace OMR

#endif // defined(BASEJBCODEGENERATOR_INCL)
