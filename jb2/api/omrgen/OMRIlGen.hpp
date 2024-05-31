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

#ifndef OMRGENILGEN_INCL
#define OMRGENILGEN_INCL

#include <stdint.h>
#include "JBCore.hpp"
#include "ilgen/IlGen.hpp"
#include "il/DataTypes.hpp"
#include "il/ILHelpers.hpp"
#include "il/ILOpCodes.hpp"

class TR_FrontEnd;
namespace TR { class Block; }
namespace TR { class CFG; }
namespace TR { class Compilation; }
namespace TR { class IlGeneratorMethodDetails; }
namespace TR { class Node; }
namespace TR { class ResolvedMethodSymbol; }
namespace TR { class SymbolReference; }
namespace TR { class SymbolReferenceTable; }
namespace TR { class TreeTop; }

namespace OMR {
namespace JitBuilder {
namespace omrgen {

class OMRCodeGenerator;

typedef TR::ILOpCodes (*OpCodeMapper)(TR::DataType);

class OMRIlGen : public TR_IlGenerator {
public:
    OMRIlGen(Compilation *comp, OMRCodeGenerator *cg);

    virtual bool genIL();
    virtual void initialize(TR::IlGeneratorMethodDetails * details,
                            TR::ResolvedMethodSymbol     * methodSymbol,
                            TR_FrontEnd                  * fe,
                            TR::SymbolReferenceTable     * symRefTab);

    virtual int32_t currentByteCodeIndex();
    virtual TR::Block *getCurrentBlock();
    virtual int32_t currentCallSiteIndex();
    virtual TR::ResolvedMethodSymbol *methodSymbol() const;

    void registerBuilder(Builder *b);
    void registerBuilder(Builder *b, int32_t bcIndex);

    void registerTypes(TypeDictionary *typedict);
    void registerNoType(const Type * t);
    void registerInt8(const Type * t);
    void registerInt16(const Type * t);
    void registerInt32(const Type * t);
    void registerInt64(const Type * t);
    void registerFloat(const Type * t);
    void registerDouble(const Type * t);
    void registerAddress(const Type * t);
    void registerPointer(const Type * t);

    void registerSymbols(SymbolDictionary *symdict);
    void createParameterSymbol(Symbol *paramSym, int32_t parameterIndex);
    void createLocalSymbol(Symbol *localSym);
    void createFunctionSymbol(Symbol *funcSym);

    void entryPoint(Builder *b);
    void genBuilder(Builder *b);
    TR::DataTypes mapType(const Type *type);

    void literalInt8(Value *resultValue, int8_t v);
    void literalInt16(Value *resultValue, int16_t v);
    void literalInt32(Value *resultValue, int32_t v);
    void literalInt64(Value *resultValue, int64_t v);
    void literalFloat(Value *resultValue, float v);
    void literalDouble(Value *resultValue, double v);
    void literalAddress(Value *resultValue, uintptr_t v);

    void add(Location *location, Value *result, Value *left, Value *right);
    void and_(Location *location, Value *result, Value *left, Value *right);
    void convertTo(Location *location, Value *result, const Type *type, Value *value, bool needsUnsigned);
    void goto_(Location *location, Builder *target);
    void ifCmpEqual(Location *location, Builder *target, Value *left, Value *right);
    void ifCmpEqualZero(Location *location, Builder *target, Value *v);
    void ifCmpGreaterThan(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned);
    void ifCmpGreaterOrEqual(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned);
    void ifCmpLessThan(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned);
    void ifCmpLessOrEqual(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned);
    void ifCmpNotEqual(Location *location, Builder *target, Value *left, Value *right);
    void ifCmpNotEqualZero(Location *location, Builder *target, Value *v);
    void indexAt(Location *location, Value *result, Value *base, const Type *elemType, Value *index);
    void load(Location *location, Value *result, Symbol *sym);
    void loadAt(Location *location, Value *result, Value *addrValue, const Type *baseType);
    void mul(Location *location, Value *result, Value *left, Value *right);
    void returnValue(Location *location, Value *value);
    void returnNoValue(Location *location);
    void store(Location *location, Symbol *sy, Value *value);
    void storeAt(Location *location, Value *addrValue, const Type *baseType, Value *valueValue);
    void sub(Location *location, Value *result, Value *left, Value *right);

protected:
    typedef struct ValueInfo {
        TR::Node *_node; // node representing Value or NULL if not generated yet
        TR::Block *_nodeBlock; // block containing definition or NULL if not generated yet
        TR::SymbolReference *_nodeSymRef; // auto sym ref or NULL if not stored into an auto
        bool _usedRemotely; // true if probably used from different block
    } ValueInfo;

    Compiler *compiler() const;
    TR::CFG *cfg() const;
    TR::SymbolReferenceTable *symRefTab() const;

    TR::Block * mapBuilder(Builder *b);

    bool endsBlock(TR::Node *n);
    TR::TreeTop * insertAsTreeTop(TR::TreeTop *tt, TR::Node *n);
    TR::SymbolReference * valueSymRef(ValueInfo *info);
    bool needsTreeTop(TR::Node *n);
    TR::Block * newBlock();
    void genBlock(TR::TreeTop *tt);
    TR::TreeTop * genTreeTop(TR::Node *n, bool fallsThrough);
    void anchorNode(TR::Node *n, List<TR::Node *> *anchored);
    void anchorFloatingNode(TR::Node *n, ValueInfo *info, bool fallsThrough);
    void genNode(TR::Node *n, bool fallsThrough);
    void defineValue(Value *v, TR::Node *n);
    TR::Node * useValue(Value *v);
    TR::Node * convertNodeTo(TR::DataType typeTo, TR::Node *n, bool needUnsigned);
    TR::Node * binaryOpNodeFromNodes(TR::ILOpCodes op, TR::Node *leftNode, TR::Node *rightNode);
    TR::Node * binaryOpFromOpMap(OpCodeMapper mapOp, TR::Node *leftNode, TR::Node *rightNode);
    void ifCmpCondition(TR_ComparisonTypes ct, bool isUnsignedCmp, TR::Node *leftNode, TR::Node *rightNode, TR::Block *targetBlock);
    TR::Node *zeroForType(TR::DataType dt);

    TR::Compilation * _comp;
    TR_FrontEnd * _fe;
    TR::SymbolReferenceTable * _symRefTab;
    TR::IlGeneratorMethodDetails * _details;
    TR::ResolvedMethodSymbol * _methodSymbol;

    int32_t _currentCallSiteIndex;
    int32_t _currentByteCodeIndex;
    int32_t _currentBlockNumber;
    
    TR::Block *_entryBlock;
    TR::Block *_exitBlock;
    TR::Block *_currentBlock;
    TR::TreeTop *_lastTree;
    TR::TreeTop *_otherBlockTrees;

    std::map<BuilderID,TR::Block *> _builderEntries;
    
    std::map<TypeID,TR::DataTypes> _types;
    BitVector _builderInTrees;

    TR::DataTypes _platformWordType;
    List<TR::Node *> _floatingNodes;
    TR::Node ** _valueNodes;
    TR::SymbolReference ** _symrefs;

    ValueInfo *_valueInfos; // indexed by value ID

    Compilation * _jb2comp;
    OMRCodeGenerator * _jb2cg;
};

} // namespace omrgen
} // namespace JitBuilder
} // namespace OMR

#endif // defined(OMRGENILGEN_INCL)
