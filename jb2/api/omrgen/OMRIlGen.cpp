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

#include "api/omrgen/OMRIlGen.hpp"
#include "api/omrgen/OMRCodeGenerator.hpp"
#include "api/omrgen/OMRCodeGeneratorExtensionAddon.hpp"

#include "codegen/CodeGenerator.hpp"
#include "compile/Compilation.hpp"
#include "compile/SymbolReferenceTable.hpp"
#include "env/FrontEnd.hpp"
#include "env/StackMemoryRegion.hpp"
#include "il/AutomaticSymbol.hpp"
#include "il/Block.hpp"
#include "il/DataTypes.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/ParameterSymbol.hpp"
#include "il/Symbol.hpp"
#include "il/TreeTop.hpp"
#include "il/TreeTop_inlines.hpp"
#include "ilgen/IlGen.hpp"
#include "ilgen/IlGeneratorMethodDetails.hpp"
#include "infra/BitVector.hpp"
#include "ras/ILValidationStrategies.hpp"
#include "ras/ILValidator.hpp"
#include <future>

namespace OMR {
namespace JitBuilder {
namespace omrgen {

OMRIlGen::OMRIlGen(Compilation *jb2comp, OMRCodeGenerator *jb2cg)
    : TR_IlGenerator ()
    , _comp(NULL)
    , _fe(NULL)
    , _symRefTab(NULL)
    , _details(NULL)
    , _methodSymbol(NULL)
    , _currentCallSiteIndex(-1)
    , _currentByteCodeIndex(-1)
    , _currentBlockNumber(-1)
    , _entryBlock(NULL)
    , _exitBlock(NULL)
    , _currentBlock(NULL)
    , _lastTree(NULL)
    , _otherBlockTrees(NULL)
    , _builderInTrees(jb2comp->mem(), jb2comp->ir()->maxBuilderID())
    , _platformWordType(TR::NoType)
    , _floatingNodes(NULL, jb2comp->mem())
    , _valueNodes(NULL)
    , _symrefs(NULL)
    , _valueInfos(NULL)
    , _jb2comp(jb2comp)
    , _jb2cg(jb2cg) {

    jb2cg->setIlGen(this);
    CodeGeneratorForExtension *cgForExt = jb2comp->ext()->addon<OMRCodeGeneratorExtensionAddon>()->cgForExtension();
    if (cgForExt != NULL) {
        cgForExt->setupbody(jb2comp);
    }

    Allocator *mem = jb2comp->mem();
    IR *ir = jb2comp->ir();
#if 0
    _builderEntries = mem->allocate<TR::Block *>(ir->maxBuilderID());
    for (auto i=0;i < ir->maxBuilderID();i++)
        _builderEntries[i] = NULL;
#endif
    ValueID numValues = ir->maxValueID() + 1;
    if (numValues > 0) {
        _valueNodes = mem->allocate<TR::Node *>(numValues);
        _valueInfos = mem->allocate<ValueInfo>(numValues);
        for (auto i=0;i < numValues;i++) {
            _valueNodes[i] = NULL;
            ValueInfo *info = _valueInfos+i;
            info->_node = NULL;
            info->_nodeBlock = NULL;
            info->_nodeSymRef = NULL;
            info->_usedRemotely = true; // TODO: set based on uses
        }
    }

    SymbolID numSymbols = ir->symdict()->numSymbols() + 1;
    if (numSymbols > 0) {
        _symrefs = mem->allocate<TR::SymbolReference *>(numSymbols);
        for (auto i=0;i < numSymbols;i++)
            _symrefs[i] = NULL;
    }
}

Compiler *
OMRIlGen::compiler() const {
    return _jb2comp->compiler();
}

bool
OMRIlGen::genIL() {
    _comp->reportILGeneratorPhase();

    TR::StackMemoryRegion stackMemoryRegion(*_comp->trMemory());

    _comp->setCurrentIlGenerator(this);
    _jb2cg->Visitor::start(_jb2comp);

    TR::Block *b = _entryBlock;
    TR::TreeTop *lastTree = b->getExit();
    while (lastTree->getNextTreeTop() != NULL) {
        b = lastTree->getNextTreeTop()->getNode()->getBlock();
        lastTree = b->getExit();
    }

    TextLogger *lgr = _jb2comp->logger();
    if (lgr) lgr->indent() << "Connecting trees" << lgr->endl();
    if (lgr) lgr->indentIn();
    for (auto i=1;i <= _jb2comp->ir()->maxBuilderID();i ++) {
        if (lgr) lgr->indent() << "Builder b" << i << ":" << lgr->endl();
        if (lgr) lgr->indentIn();
        if (_builderEntries[i] != NULL && _builderInTrees[i] != true) {
            _builderInTrees.setBit(i);
            TR::Block *b = _builderEntries[i];
            TR::TreeTop *entry = b->getEntry();
            if (lgr) lgr->indent() << "Tacking entry " << (void *) entry->getNode() << " after lastTree " << (void *) lastTree->getNode() << lgr->endl();
            lastTree->setNextTreeTop(entry);
            entry->setPrevTreeTop(lastTree);
            TR::TreeTop *lastTT = b->getExit();
            if (lgr) lgr->indent() << "Block ends with lastTT " << (void *) lastTT->getNode() << lgr->endl();
            while (lastTT->getNextTreeTop() != NULL) {
                TR::TreeTop *nextTT = lastTT->getNextTreeTop();
                b = nextTT->getNode()->getBlock();
                if (lgr) lgr->indent() << "Found following block BB" << b->getNumber() << lgr->endl();
                lastTT = b->getExit();
                if (lgr) lgr->indent() << "ends at TT " << lastTT->getNode() << lgr->endl();
            }
            lastTree = lastTT;
            if (lgr) lgr->indent() << "Updated lastTree " << (void *) lastTree->getNode() << lgr->endl();
        }
        if (lgr) lgr->indentOut();
    }
    if (lgr) lgr->indentOut();
    if (_exitBlock) {
        lastTree->setNextTreeTop(_exitBlock->getEntry());
        _exitBlock->getEntry()->setPrevTreeTop(lastTree);
        lastTree = _exitBlock->getExit();
    }

    _comp->setCurrentIlGenerator(0);

#if !defined(DISABLE_CFG_CHECK)
    if (_comp->getOption(TR_UseILValidator)) {
        /* Setup the ILValidator for the current Compilation Thread. */
        _comp->setILValidator(createILValidatorObject(_comp));
    }
#endif
    _comp = NULL;

    return true;
}

void
OMRIlGen::initialize(TR::IlGeneratorMethodDetails * details,
                     TR::ResolvedMethodSymbol     * methodSymbol,
                     TR_FrontEnd                  * fe,
                     TR::SymbolReferenceTable     * symRefTab) {
    _details = details;
    _methodSymbol = methodSymbol;
    _fe = fe;
    _symRefTab = symRefTab;
    _comp = TR::comp();
}

int32_t
OMRIlGen::currentCallSiteIndex() {
    return _currentCallSiteIndex;
}

int32_t
OMRIlGen::currentByteCodeIndex() {
    return _currentByteCodeIndex;
}

TR::Block *
OMRIlGen::getCurrentBlock() {
    return _currentBlock;
}

TR::ResolvedMethodSymbol *
OMRIlGen::methodSymbol() const {
    return _methodSymbol;
}

TR::CFG *
OMRIlGen::cfg() const {
    return _methodSymbol->getFlowGraph();
}

TR::SymbolReferenceTable *
OMRIlGen::symRefTab() const {
    return _symRefTab;
}

void
OMRIlGen::registerBuilder(Builder *b) {
    BuilderID id = b->id();
    if (_builderEntries.find(id) == _builderEntries.end()) {
        TR::Block *tr_b = newBlock();
        _builderEntries.insert({id, tr_b});
    }
}

void
OMRIlGen::registerTypes(TypeDictionary *typedict) {
    TypeID numTypes = typedict->numTypes();
    Allocator myMem("Type mapping", _jb2comp->mem());
    BitVector mappedTypes(&myMem, numTypes);
    while (numTypes > 0) {
        TypeID startNumTypes = numTypes;
        for (auto it = typedict->typesIterator(); it.hasItem(); it++) {
            const Type *type = it.item();
            if (mappedTypes.getBit(type->id()) != true) {
                CodeGeneratorForExtension *cgForExt = type->ext()->addon<OMRCodeGeneratorExtensionAddon>()->cgForExtension();
                bool mapped = cgForExt->registerType(type);
                if (mapped) {
                    numTypes--;
                    mappedTypes.setBit(type->id());
                }
            }
        }
        assert(numTypes < startNumTypes);
    }
}

void
OMRIlGen::registerNoType(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::NoType;
}

void
OMRIlGen::registerInt8(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::Int8;
}

void
OMRIlGen::registerInt16(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::Int16;
}

void
OMRIlGen::registerInt32(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::Int32;
    if (compiler()->platformWordSize() == 32)
        _platformWordType = TR::DataTypes::Int32;
}

void
OMRIlGen::registerInt64(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::Int64;
    if (compiler()->platformWordSize() == 64)
        _platformWordType = TR::DataTypes::Int64;
}

void
OMRIlGen::registerFloat(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::Float;
}

void
OMRIlGen::registerDouble(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::Double;
}

void
OMRIlGen::registerAddress(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = TR::DataTypes::Address;
}

TR::Block *
OMRIlGen::mapBuilder(Builder *b) {
    auto it = _builderEntries.find(b->id());
    if (it == _builderEntries.end())
        return NULL;
    return it->second;
}

TR::DataTypes
OMRIlGen::mapType(const Type *type) {
    auto it = _types.find(type->id());
    if (it == _types.end())
        return TR::NoType;
    return it->second;
}

void
OMRIlGen::registerSymbols(SymbolDictionary *symdict) {
    for (auto it = symdict->symbolIterator(); it.hasItem(); it++) {
        Symbol *sym = it.item();
        CodeGeneratorForExtension *cgForExt = sym->ext()->addon<OMRCodeGeneratorExtensionAddon>()->cgForExtension();
        if (cgForExt)
            assert(cgForExt->registerSymbol(sym));
        else
            assert(_jb2cg->registerSymbol(sym));
    }
}

void
OMRIlGen::createLocalSymbol(Symbol *localSym) {
    TR::DataTypes dt = mapType(localSym->type());
    TR::SymbolReference *symRef = _symrefs[localSym->id()] = _symRefTab->createTemporary(_methodSymbol, dt);
    TR::Symbol *sym = symRef->getSymbol();
    sym->getAutoSymbol()->setName(localSym->name().c_str());
    if (!localSym->type()->isManaged())
        sym->setNotCollected();
}

void
OMRIlGen::createParameterSymbol(Symbol *parameterSym, int32_t parameterIndex) {
    TR::DataTypes dt = mapType(parameterSym->type());
    TR::SymbolReference *symRef = _symrefs[parameterSym->id()] = _symRefTab->findOrCreateAutoSymbol(_methodSymbol, parameterIndex, dt, true, false, true);
    TR::Symbol *sym = symRef->getSymbol();
    sym->getParmSymbol()->setName(parameterSym->name().c_str());
    if (!parameterSym->type()->isManaged())
        sym->setNotCollected();
}

void
OMRIlGen::genBuilder(Builder *b) {
    _currentBlock = mapBuilder(b);
    assert(_currentBlock != NULL);

    TextLogger *lgr = _jb2comp->logger();
    if (lgr)
        lgr->indent() << "Generating builder " << b << " [ to TR::Block BB" << _currentBlock->getNumber() << "]" << lgr->endl();
}

//
// Node and Treetop creation follows these relatively simple rules

// Value use
//    either load of an auto, added to floating nodes
//    or a remembered node (already anchored or floating)
//    there are no treetops
//
// Value def of a node
//    if used remotely, put under a store and anchor floating children
//    or if isn’t a treetop but needs not, put under a treetop and anchor floating children
//    or add it to floating nodes
//
// Node created while evaluating a value
//    if it needs a treetop, create a treetop node and then use that
//    if it is a treetop node, create a treetop and anchor floating children
//    if has control flow and falls through
//        create new block along with fall-through edge and update _currentBlock
//
// Operation translators should follow this basic model:
//    First, "use" all operands
//    Then execute the steps of the operation using the TR::Nodes produced by using the operands
//    Finally all results are "defined"

bool
OMRIlGen::endsBlock(TR::Node *n) {
    return (n->getOpCode().isReturn() ||
        n->getOpCode().isBranch() ||
        n->getOpCode().isIf() ||
        n->getOpCode().isGoto());
}

TR::TreeTop *
OMRIlGen::insertAsTreeTop(TR::TreeTop *tt, TR::Node *n) {
    if (!n->getOpCode().isTreeTop())
        n = TR::Node::create(TR::treetop, 1, n);
    TR::TreeTop *newTT = TR::TreeTop::create(TR::comp(), n);
    tt->insertBefore(newTT);
    return newTT;
}

TR::SymbolReference *
OMRIlGen::valueSymRef(ValueInfo *info) {
    TR::SymbolReference *sr = info->_nodeSymRef;
    if (sr == NULL)
        sr = info->_nodeSymRef = symRefTab()->createTemporary(methodSymbol(), info->_node->getDataType());
    return sr;
}

bool
OMRIlGen::needsTreeTop(TR::Node *n) {
    return (n->getOpCode().isTreeTop());
}

TR::Block *
OMRIlGen::newBlock() {
    TR::Block *b = TR::Block::createEmptyBlock(TR::comp());
    cfg()->addNode(b);
    return b;
}

void
OMRIlGen::genBlock(TR::TreeTop *tt) {
    // allocate a new empty block and append to this one and then set as current
    TR::Block *b = newBlock();
    _currentBlock->getExit()->join(b->getEntry());
    cfg()->addEdge(_currentBlock, b);
    _currentBlock = b;
}

TR::TreeTop *
OMRIlGen::genTreeTop(TR::Node *n, bool fallsThrough) {
    if (_currentBlock == NULL)
        _currentBlock = newBlock();

    if (!n->getOpCode().isTreeTop()) {
        n = TR::Node::create(TR::treetop, 1, n);
    }

    TR::TreeTop *tt = _currentBlock->append(TR::TreeTop::create(TR::comp(), n));
    if (fallsThrough) {
        if (endsBlock(n))
            genBlock(tt);
    } else {
        _currentBlock = NULL;
    }

    return tt;
}

void
OMRIlGen::anchorNode(TR::Node *n, List<TR::Node *> *anchored) {
    anchored->push_front(n);
    for (auto c=0;c < n->getNumChildren();c++) {
        TR::Node *child = n->getChild(c);
        if (_floatingNodes.find(child).hasItem())
            anchorNode(child, anchored);
    }
}

// would be nice if this was better than O(n^2) though n is usually small (hopefully)
void
OMRIlGen::anchorFloatingNode(TR::Node *n, ValueInfo *info, bool fallsThrough) {
    List<TR::Node *> anchored(NULL, _jb2comp->mem());

    if (info != NULL) {
        TR::SymbolReference *sr = valueSymRef(info);
        n = TR::Node::createStore(sr, n);
    }
    TR::TreeTop *tt = genTreeTop(n, fallsThrough);
    anchorNode(n, &anchored); // anchor n and any floating nodes in its tree of children

    // iterate through floating nodes and make sure they are all anchored
    for (auto it=_floatingNodes.revIterator(); it.hasItem(); it++) {
        TR::Node *n = it.item();
        assert(n != NULL);
        if (!anchored.find(n).hasItem()) {
            tt = insertAsTreeTop(tt, n);
            anchorNode(n, &anchored);
        }
    }

    // all floating nodes are anchored so clear the list
    _floatingNodes.erase();
}

void
OMRIlGen::genNode(TR::Node *n, bool fallsThrough) {
    if (needsTreeTop(n)) {
        anchorFloatingNode(n, NULL, fallsThrough);
    } else {
        _floatingNodes.push_back(n);
    }
}

void
OMRIlGen::defineValue(Value *v, TR::Node *n) {
    ValueInfo *info = _valueInfos + v->id();
    assert(info->_node == NULL);
    info->_node = n;
    if (info->_usedRemotely) {
        anchorFloatingNode(n, info, true);
    } else {
        genNode(n, true);
    }
}

void
OMRIlGen::entryPoint(Builder *b) {
    assert(_entryBlock == NULL);

    _entryBlock = mapBuilder(b);
    assert(_entryBlock != NULL);
    cfg()->addEdge(cfg()->getStart(), _entryBlock);
    _methodSymbol->setFirstTreeTop(_entryBlock->getEntry());

    _currentBlock = _entryBlock;

    _builderInTrees.setBit(b->id());
}

TR::Node *
OMRIlGen::useValue(Value *v) {
    ValueInfo *info = _valueInfos + v->id();
    if (info->_nodeBlock == _currentBlock)
        return info->_node;
    TR::SymbolReference *sr = info->_nodeSymRef;
    if (sr == NULL) {
        sr = info->_nodeSymRef = symRefTab()->createTemporary(methodSymbol(), mapType(v->type()));
    }
    return TR::Node::createLoad(sr);
}

void
OMRIlGen::literalInt8(Value *resultValue, int8_t v) {
    defineValue(resultValue, TR::Node::bconst(v));
}

void
OMRIlGen::literalInt16(Value *resultValue, int16_t v) {
    defineValue(resultValue, TR::Node::sconst(v));
}
void
OMRIlGen::literalInt32(Value *resultValue, int32_t v) {
    defineValue(resultValue, TR::Node::iconst(v));
}
void
OMRIlGen::literalInt64(Value *resultValue, int64_t v) {
    defineValue(resultValue, TR::Node::lconst(v));
}
void
OMRIlGen::literalFloat(Value *resultValue, float v) {
    TR::Node *constNode = TR::Node::create(0, TR::fconst, 0);
    constNode->setFloat(v);
    defineValue(resultValue, constNode);
}

void
OMRIlGen::literalDouble(Value *resultValue, double v) {
    TR::Node *constNode = TR::Node::create(0, TR::dconst, 0);
    constNode->setDouble(v);
    defineValue(resultValue, constNode);
}

void
OMRIlGen::literalAddress(Value *resultValue, uintptr_t v) {
    defineValue(resultValue, TR::Node::aconst(v));
}

TR::Node *
OMRIlGen::convertNodeTo(TR::DataType typeTo, TR::Node *n, bool needUnsigned) {
    TR::DataType typeFrom = n->getDataType();
    if (typeFrom == typeTo) {
        return n;
    }

    TR::ILOpCodes convertOp = ILOpCode::getProperConversion(typeFrom, typeTo, needUnsigned);
    if (convertOp == TR::BadILOp ||
        !_comp->cg()->isILOpCodeSupported(convertOp) ||
        (typeFrom.isIntegral() && typeFrom != _platformWordType && !typeTo.isIntegral()) || // compensate for some common failures of isILOpCodeSupported for this scenario
        (!typeFrom.isIntegral() && typeTo.isIntegral() && (typeTo == TR::Int8 || typeTo == TR::Int16))) {

        TR::ILOpCodes intermediateOp = ILOpCode::getProperConversion(typeFrom, _platformWordType, needUnsigned);
        n = TR::Node::create(intermediateOp, 1, n);
        genNode(n, true);
        convertOp = ILOpCode::getProperConversion(_platformWordType, typeTo, needUnsigned);
    }
    TR_ASSERT_FATAL(convertOp != TR::BadILOp, "Unknown conversion requested for node %p %s to %s", this, n, typeFrom.toString(), typeTo.toString());
   
    TR::Node *convertedValue = TR::Node::create(convertOp, 1, n);
    return convertedValue;
}

TR::Node *
OMRIlGen::binaryOpNodeFromNodes(TR::ILOpCodes op, TR::Node *leftNode, TR::Node *rightNode) {
    TR::DataType leftType = leftNode->getDataType();
    TR::DataType rightType = rightNode->getDataType();
    bool isAddressBump = ((leftType == TR::Address) &&
                          (rightType == TR::Int32 || rightType == TR::Int64));
    bool isRevAddressBump = ((rightType == TR::Address) &&
                             (leftType == TR::Int32 || leftType == TR::Int64));
    TR_ASSERT_FATAL(leftType == rightType || isAddressBump || isRevAddressBump, "binaryOp requires both left and right operands to have same type or one is address and other is Int32/64");

    if (isRevAddressBump) { // swap them
        TR::Node *save = leftNode;
        leftNode = rightNode;
        rightNode = save;
    }

    return TR::Node::create(op, 2, leftNode, rightNode);
}

TR::Node *
OMRIlGen::binaryOpFromOpMap(OpCodeMapper mapOp, TR::Node *leftNode, TR::Node *rightNode) {
    TR::DataType leftType = leftNode->getDataType();
    return binaryOpNodeFromNodes(mapOp(leftType), leftNode, rightNode);
}

static TR::ILOpCodes addOpCode(TR::DataType type) {
    return TR::ILOpCode::addOpCode(type, TR::Compiler->target.is64Bit());
}

void
OMRIlGen::add(Location *location, Value *result, Value *left, Value *right) {
    TR::DataTypes leftType = mapType(left->type());
    TR::Node *leftNode = useValue(left);
    TR::DataTypes rightType = mapType(right->type());
    TR::Node *rightNode = useValue(right);

    TR::Node *resultNode = NULL;
    if (leftType == TR::Address) {
        if (compiler()->platformWordSize() == 64 && rightType == TR::Int32) {
            rightNode = TR::Node::create(TR::i2l, 1, rightNode);
        }
        else if (compiler()->platformWordSize() == 32 && rightType == TR::Int64) {
            rightNode = TR::Node::create(TR::l2i, 1, rightNode);
        }
        resultNode = binaryOpNodeFromNodes(TR::Compiler->target.is32Bit() ? TR::aiadd : TR::aladd, leftNode, rightNode);
    } else {
        resultNode = binaryOpFromOpMap(addOpCode, leftNode, rightNode);
    }
    //TraceIL("IlBuilder[ %p ]::%d is Add %d + %d\n", this, returnValue->getID(), left->getID(), right->getID());
    defineValue(result, resultNode);
}

void
OMRIlGen::and_(Location *location, Value *result, Value *left, Value *right) {
    TR::Node *leftNode = useValue(left);
    TR::Node *rightNode = useValue(right);

    TR::Node *resultNode = NULL;
    resultNode = binaryOpFromOpMap(TR::ILOpCode::andOpCode, leftNode, rightNode);
    defineValue(result, resultNode);
}

void
OMRIlGen::convertTo(Location *location, Value *result, const Type *typeTo, Value *value, bool needUnsigned) {
    TR::Node *valueNode = useValue(value);
    TR::Node *convertedValue = convertNodeTo(mapType(typeTo), valueNode, needUnsigned);
    defineValue(result, convertedValue);
}

void
OMRIlGen::div(Location *location, Value *result, Value *left, Value *right) {
    TR::DataTypes leftType = mapType(left->type());
    TR::Node *leftNode = useValue(left);
    TR::DataTypes rightType = mapType(right->type());
    TR::Node *rightNode = useValue(right);
    TR::Node *resultNode = resultNode = binaryOpFromOpMap(TR::ILOpCode::divideOpCode, leftNode, rightNode);
    defineValue(result, resultNode);
}

void
OMRIlGen::goto_(Location *location, Builder *target) {
    TR::Block *targetBlock = mapBuilder(target);
    TR::Node *gotoNode = TR::Node::create(NULL, TR::Goto);
    gotoNode->setBranchDestination(targetBlock->getEntry());
    cfg()->addEdge(_currentBlock, targetBlock);
    genTreeTop(gotoNode, false);
}

TR::Node *
OMRIlGen::zeroForType(TR::DataType dt) {
    switch (dt) {
        case TR::Int8 :  return TR::Node::bconst(0);
        case TR::Int16 : return TR::Node::sconst(0);
        case TR::Int32 : return TR::Node::iconst(0);
        case TR::Int64 : return TR::Node::lconst(0);
        case TR::Float : {
            TR::Node *constZero = TR::Node::create(TR::fconst, 0);
            constZero->setFloatBits(FLOAT_POS_ZERO);
            return constZero;
        }
        case TR::Double : {
            TR::Node *constZero = TR::Node::create(TR::dconst, 0);
            constZero->setUnsignedLongInt(DOUBLE_POS_ZERO);
        }
        case TR::Address : return TR::Node::aconst(0);
        default: TR_ASSERT_FATAL(0, "should not reach here");
    }
}

void
OMRIlGen::ifCmpCondition(TR_ComparisonTypes ct, bool isUnsigned, TR::Node *leftNode, TR::Node *rightNode, TR::Block *targetBlock) {
    TR::DataType dt = leftNode->getDataType();
    TR::ILOpCode cmpOpCode(TR::ILOpCode::compareOpCode(dt, ct, isUnsigned));

    // some unpleasantness because not all platforms currently implement all(any?) 8 or 16 bit ifcmp opcodes
    if ((dt == TR::Int8 && !compiler()->platformImplements8bCompares()) ||
        (dt == TR::Int16 && !compiler()->platformImplements16bCompares())) {

        leftNode = convertNodeTo(TR::Int32, leftNode, isUnsigned);
        rightNode = convertNodeTo(TR::Int32, rightNode, isUnsigned);
        cmpOpCode = TR::ILOpCode::compareOpCode(TR::Int32, ct, isUnsigned);
    }

    cfg()->addEdge(_currentBlock, targetBlock);
    TR::Node *ifNode = TR::Node::createif(cmpOpCode.convertCmpToIfCmp(), leftNode, rightNode, targetBlock->getEntry());
    genTreeTop(ifNode, true);
}

void
OMRIlGen::ifCmpEqual(Location *location, Builder *target, Value *left, Value *right) {
    TR::Node *leftNode = useValue(left);
    TR::Node *rightNode = useValue(right);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpEQ, false, leftNode, rightNode, targetBlock);
}

void
OMRIlGen::ifCmpEqualZero(Location *location, Builder *target, Value *v) {
    TR::Node *condition = useValue(v);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpEQ, false, condition, zeroForType(condition->getDataType()), targetBlock);
}

void
OMRIlGen::ifCmpGreaterThan(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned) {
    TR::Node *leftNode = useValue(left);
    TR::Node *rightNode = useValue(right);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpGT, isUnsigned, leftNode, rightNode, targetBlock);
}

void
OMRIlGen::ifCmpGreaterOrEqual(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned) {
    TR::Node *leftNode = useValue(left);
    TR::Node *rightNode = useValue(right);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpGE, isUnsigned, leftNode, rightNode, targetBlock);
}

void
OMRIlGen::ifCmpLessThan(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned) {
    TR::Node *leftNode = useValue(left);
    TR::Node *rightNode = useValue(right);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpLT, isUnsigned, leftNode, rightNode, targetBlock);
}

void
OMRIlGen::ifCmpLessOrEqual(Location *location, Builder *target, Value *left, Value *right, bool isUnsigned) {
    TR::Node *leftNode = useValue(left);
    TR::Node *rightNode = useValue(right);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpLE, isUnsigned, leftNode, rightNode, targetBlock);
}

void
OMRIlGen::ifCmpNotEqual(Location *location, Builder *target, Value *left, Value *right) {
    TR::Node *leftNode = useValue(left);
    TR::Node *rightNode = useValue(right);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpNE, false, leftNode, rightNode, targetBlock);
}

void
OMRIlGen::ifCmpNotEqualZero(Location *location, Builder *target, Value *v) {
    TR::Node *condition = useValue(v);
    TR::Block *targetBlock = mapBuilder(target);
    ifCmpCondition(TR_cmpNE, false, condition, zeroForType(condition->getDataType()), targetBlock);
}

void
OMRIlGen::indexAt(Location *location, Value *result, Value *base, const Type *elementType, Value *index) {
    TR::Node *baseNode = useValue(base);
    TR::Node *indexNode = useValue(index);
    TR::Node *elemSizeNode;
    TR::ILOpCodes addOp, mulOp;
    TR::DataType indexType = indexNode->getDataType();
    if (_jb2comp->compiler()->platformWordSize() == 64) {
        if (indexType != TR::Int64) {
            TR::ILOpCodes op = TR::ILOpCode::getDataTypeConversion(indexType, TR::Int64);
            indexNode = TR::Node::create(op, 1, indexNode);
        }
        elemSizeNode = TR::Node::lconst(elementType->size() / 8);
        addOp = TR::aladd;
        mulOp = TR::lmul;
    } else {
        TR::DataType targetType = TR::Int32;
        if (indexType != targetType) {
            TR::ILOpCodes op = TR::ILOpCode::getDataTypeConversion(indexType, targetType);
            indexNode = TR::Node::create(op, 1, indexNode);
        }
        elemSizeNode = TR::Node::iconst(static_cast<int32_t>(elementType->size() / 8));
        addOp = TR::aiadd;
        mulOp = TR::imul;
    }

    TR::Node *offsetNode = TR::Node::create(mulOp, 2, indexNode, elemSizeNode);
    TR::Node *addrNode = TR::Node::create(addOp, 2, baseNode, offsetNode);
    defineValue(result, addrNode);
}

void
OMRIlGen::load(Location *location, Value *result, Symbol *sym) {
    TR::SymbolReference *symref = _symrefs[sym->id()];
    assert(symref != NULL);
    TR::Node *loadNode = TR::Node::createLoad(symref);
    defineValue(result, loadNode);
}

void
OMRIlGen::loadAt(Location *location, Value *result, Value *addrValue, const Type *baseType) {
    TR_ASSERT_FATAL(mapType(addrValue->type()) == TR::Address, "loadAt needs an address operand");
    TR::Node *addrNode = useValue(addrValue);
    TR_ASSERT_FATAL(addrNode->getDataType() == TR::Address, "LoadAt needs an address operand");
    TR::DataType addrBaseType = mapType(baseType); // assumes addrValue's Type dereferences to value of baseType
    TR::SymbolReference *loadSymRef = symRefTab()->findOrCreateArrayShadowSymbolRef(addrBaseType, addrNode);
    TR::ILOpCodes loadOp = _comp->il.opCodeForIndirectArrayLoad(addrBaseType);
    TR::Node *loadNode = TR::Node::createWithSymRef(loadOp, 1, 1, addrNode, loadSymRef);
    defineValue(result, loadNode);
}

void
OMRIlGen::mul(Location *location, Value *result, Value *left, Value *right) {
    TR::DataTypes leftType = mapType(left->type());
    TR::Node *leftNode = useValue(left);
    TR::DataTypes rightType = mapType(right->type());
    TR::Node *rightNode = useValue(right);
    TR::Node *resultNode = resultNode = binaryOpFromOpMap(TR::ILOpCode::multiplyOpCode, leftNode, rightNode);
    defineValue(result, resultNode);
}

void
OMRIlGen::returnValue(Location *location, Value *value) {
    TR::Node *valueNode = useValue(value);
    assert(valueNode != NULL);

    TR::DataType retType = valueNode->getDataType();
    if (retType == TR::Int8 || retType == TR::Int16 || (retType == TR::Int32 && _platformWordType == TR::Int64)) {
        retType = _platformWordType;
        valueNode = convertNodeTo(retType, valueNode, false); // hm, how can "false" always be right? need returnUnsigned variants?
    }

    TR::Node *returnNode = TR::Node::create(TR::ILOpCode::returnOpCode(retType), 1, valueNode);
    cfg()->addEdge(_currentBlock, cfg()->getEnd());
    genNode(returnNode, false);
}

void
OMRIlGen::returnNoValue(Location *location) {
    TR::Node *returnNode = TR::Node::create(TR::ILOpCode::returnOpCode(TR::NoType));
    cfg()->addEdge(_currentBlock, cfg()->getEnd());
    genNode(returnNode, false);
}

void
OMRIlGen::store(Location *location, Symbol *sym, Value *value) {
    TR::SymbolReference *symref = _symrefs[sym->id()];
    assert(symref != NULL);
    TR::Node *valueNode = useValue(value);
    assert(valueNode != NULL);
    TR::Node *storeNode = TR::Node::createStore(symref, valueNode);
    genNode(storeNode, true);
}

void
OMRIlGen::storeAt(Location *location, Value *addrValue, const Type *baseType, Value *valueValue) {
   TR_ASSERT_FATAL(mapType(addrValue->type()) == TR::Address, "storeAt needs an address operand");
    TR::Node *addrNode = useValue(addrValue);
    TR_ASSERT_FATAL(addrNode->getDataType() == TR::Address, "StoreAt needs an address operand");
    TR::Node *valueNode = useValue(valueValue);
    TR::DataType addrBaseType = mapType(baseType); // assumes addrValue's Type dereferences to value of baseType
    TR_ASSERT_FATAL(addrBaseType == valueNode->getDataType(), "StoreAt address base type and value type should match");
    TR::SymbolReference *storeSymRef = symRefTab()->findOrCreateArrayShadowSymbolRef(addrBaseType, addrNode);
    TR::ILOpCodes storeOp = _comp->il.opCodeForIndirectArrayStore(addrBaseType);
    TR::Node *storeNode = TR::Node::createWithSymRef(storeOp, 2, addrNode, valueNode, 0, storeSymRef);
    genNode(storeNode, true);
}

void
OMRIlGen::sub(Location *location, Value *result, Value *left, Value *right) {
    TR::Node *leftNode = useValue(left);
    TR::DataType leftType = leftNode->getDataType();
    TR::Node *rightNode = useValue(right);
    TR::DataType rightType = leftNode->getDataType();
    TR::Node *resultNode = NULL;

    if (leftType == TR::Address) {
        TR::ILOpCodes op;
        bool needSub=true;
        if (TR::Compiler->target.is64Bit()) {
            op = TR::aladd;
            if (rightType == TR::Int32) {
                rightNode = TR::Node::create(TR::i2l, 1, rightNode);
                rightNode = TR::Node::create(TR::lsub, 2, TR::Node::lconst(0), rightNode);
            } else if (rightType == TR::Address) {
                leftNode = TR::Node::create(TR::a2l, 1, leftNode);
                rightNode = TR::Node::create(TR::a2l, 1, rightNode);
                op = TR::lsub;
            }
        } else if (TR::Compiler->target.is32Bit()) {
            op = TR::aiadd;
            if (rightType == TR::Int64) {
                rightNode = TR::Node::create(TR::l2i, 1, rightNode);
                rightNode = TR::Node::create(TR::isub, 2, TR::Node::iconst(0), rightNode);
            } else if (rightType == TR::Address) {
                leftNode = TR::Node::create(TR::a2i, 1, leftNode);
                rightNode = TR::Node::create(TR::a2i, 1, rightNode);
                op = TR::isub;
            }
        }
      resultNode = binaryOpNodeFromNodes(op, leftNode, rightNode);
    } else {
        resultNode = binaryOpFromOpMap(TR::ILOpCode::subtractOpCode, leftNode, rightNode);
    }

    defineValue(result, resultNode);
}

} // namespace omrgen
} // namespace JitBuilder
} // namespace OMR
