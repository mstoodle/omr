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
#include "jbgen/JBCodeGenerator.hpp"
#include "jbgen/JBMethodBuilder.hpp"
#include "jbgen/JBCodeGeneratorExtensionAddon.hpp"

#include "ilgen/MethodBuilder.hpp"
#include "ilgen/IlType.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "ilgen/VirtualMachineState.hpp"

extern int32_t internal_compileMethodBuilder(TR::MethodBuilder *mb, void ** entryPoint);

namespace OMR {
namespace JB2 {
namespace jbgen {

INIT_JBALLOC_REUSECAT(JBCodeGenerator, CodeGeneration);
SUBCLASS_KINDSERVICE_IMPL(JBCodeGenerator,"JBCodeGenerator",CodeGenerator,Extensible);

JBCodeGenerator::JBCodeGenerator(Allocator *a, Extension *ext)
    : CodeGenerator(a, CLASSKIND(JBCodeGenerator, Extensible), ext, "jbcg")
    , _jbmb(NULL)
    , _jbCompileReturnCode(0)
    , _compileReturnCode(ext->compiler()->CompileNotStarted) {
    setTraceEnabled(false);
}

JBCodeGenerator::~JBCodeGenerator() {
}

CompilerReturnCode
JBCodeGenerator::perform(Compilation *comp) {
    class CompileMethodBuilder : public TR::MethodBuilder {
    public:
        CompileMethodBuilder(Compilation *comp, JBCodeGenerator *cg, TR::TypeDictionary *types)
            : TR::MethodBuilder(types)
            , _comp(comp)
            , _cg(cg) {

            JBMethodBuilder *jbmb = cg->jbmb();
            jbmb->setMethodBuilder(this);
            jbmb->registerTypes(_comp->ir()->typedict());

            CodeGeneratorForExtension *cgForExt = comp->ext()->addon<JBCodeGeneratorExtensionAddon>()->cgForExtension();
            if (cgForExt) {
                cgForExt->setupbody(comp);
            } else {
                cg->setupbody(comp);
            }
        }

        virtual bool buildIL() {
            TR::VirtualMachineState prototype;
            setVMState(prototype.MakeCopy());
            JBMethodBuilder *jbmb = _cg->jbmb();
            _cg->Visitor::start(_comp);
            return true;
        }

    protected:
        Compilation * _comp;
        JBCodeGenerator * _cg;
    };

   // setTraceEnabled(true); //comp->config()->traceCodeGenerator());
    _jbCompileReturnCode=-1;

    if (comp->logger()) {
        TextLogger &logger = *comp->logger();
        TextWriter *wrt = new (comp->mem()) TextWriter(comp->mem(), comp->compiler(), logger);
        wrt->perform(comp);
        delete wrt;
    }

    void *entryPoint=NULL;
    {
        JBMethodBuilder jbmb(comp);
        _jbmb = &jbmb;

        TR::TypeDictionary types;
        CompileMethodBuilder cmb(comp, this, &types);

        entryPoint=NULL;
        _jbCompileReturnCode = internal_compileMethodBuilder(&cmb, &entryPoint);

        _jbmb = NULL;
    }
    setTraceEnabled(false);

    if (_jbCompileReturnCode != 0)
        _compileReturnCode = comp->compiler()->CompileFailed;

    Allocator *mem = comp->compiler()->mem();
    EntryID eid = 0;
    NativeEntry *entry = new (mem) NativeEntry(mem, comp->ir(), eid, entryPoint);
    comp->scope<Scope>()->addEntryPoint(entry, eid);

    _compileReturnCode = comp->compiler()->CompileSuccessful;
    return _compileReturnCode;
}

void
JBCodeGenerator::visitPreCompilation(Compilation *comp) {
    // create all the appropriate builder objects ahead of time
    for (auto it = comp->builders();it.hasItem();it++) {
        Builder *b = it.item();
        CodeGeneratorForExtension *cgForExt = b->ext()->addon<JBCodeGeneratorExtensionAddon>()->cgForExtension();
        if (cgForExt) {
            cgForExt->registerBuilder(b);
        } else { // assume it's just a Builder
            _jbmb->createBuilder(b);
        }
    }
    CodeGeneratorForExtension *cgForExt = comp->ext()->addon<JBCodeGeneratorExtensionAddon>()->cgForExtension();
    cgForExt->genbody(comp);
}

Builder *
JBCodeGenerator::gencode(Operation *op) {
    assert(_jbmb);
    CodeGeneratorForExtension *cgForExt = op->ext()->addon<JBCodeGeneratorExtensionAddon>()->cgForExtension();
    if (cgForExt) {
        cgForExt->gencode(op);
    }
    return NULL;
}

void
JBCodeGenerator::visitBuilderPostOps(Builder *b) {
    assert(_jbmb);
    CodeGeneratorForExtension *cgForExt = b->ext()->addon<JBCodeGeneratorExtensionAddon>()->cgForExtension();
    if (cgForExt) {
        cgForExt->connectsuccessors(b);
    }
}

void
JBCodeGenerator::visitOperation(Operation * op) {
    assert(_jbmb);

    TextLogger *logger = NULL; //_comp->logger();
    if (logger) {
        TextLogger &log = *logger;
        log << op << log.endl();
        for (auto oit=op->operands(); oit.hasItem(); oit++) {
            Value *v = oit.item();
            log << "    consumes " << v << " : " << _jbmb->map(v) << log.endl();
        }
    }

    CodeGeneratorForExtension *cgForExt = op->ext()->addon<JBCodeGeneratorExtensionAddon>()->cgForExtension();
    if (cgForExt) {
        cgForExt->gencode(op);
    }

    if (logger) {
        TextLogger &log = *logger;
        for (auto rit=op->results(); rit.hasItem(); rit++) {
            Value *v = rit.item();
            log << "    produces " << v << " : " << _jbmb->map(v) << log.endl();
        }
    }
}

void
JBCodeGenerator::visitPostCompilation(Compilation *comp) {
    assert(_jbmb);
    TextLogger *lgr = NULL; //comp->logger(traceEnabled());
    if (lgr) {
        lgr->indentOut();
        _jbmb->printAllMaps();
    }
}

#if 0
// old impl of JBCodeGenerator kept here so it's handy during mgration

void
JBCodeGenerator::generateFunctionAPI(FunctionBuilder *fb) {
    TextLogger *lgr = fb->logger(traceEnabled());
    if (lgr) lgr->indent() << "JBCodeGenerator::generateFunctionAPI F" << fb->id() << lgr->endl();

    TR::TypeDictionary * typesJB = _mb->typeDictionary();
    _typeDictionaries[types->id()] = typesJB;

    TypeDictionary *dict = _comp->dict();
    TypeID numTypes = _comp->numTypes();
    while (numTypes > 0) {
        TypeID startNumTypes = numTypes;
        for (auto it = dict->TypesBegin(); it !+ dict->TypeEnd(); it++) {
            Type *type = *it;
            if (_types[type->id()] == NULL) {
                bool mapped = type->registerJBType(this)
                if (mapped) {
                    assert(_types[type->id()] != NULL);
                    numTypes--;
                }
            }
        }
        assert(numTypes < startNumTypes);
    }

    if (lgr) lgr->indent() << "First pass:" << lgr->endl();
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        if (lgr) {
            lgr->indent();
            lgr->logType(type, true);
            lgr << lgr->endl();
        }
        if (type->isStruct() || type->isUnion()) {
            char *name = findOrCreateString(type->name());
            storeType(type, _mb->typeDictionary()->DefineStruct(name));
        }
        else if (type->isFunction()) {
            // function types all map to Address in JB2 1.0
            storeType(type, _mb->typeDictionary()->Address);
        }
        else if (type->isPointer() || type->isField()) {
            // skip function and pointer types in this pass
        }
        else {
            // should be a primitive type; verify that the type has been mapped already
            //assert(mapType(type));
        }
    }

    // Second pass: map all Pointer types now that anything a Pointer can point to has been mapped
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        if (type->isPointer()) {
            TR::IlType *ptrIlType = mapPointerType(typesJB, static_cast<PointerType *>(type));
            storeType(type, ptrIlType);
        }
    }

    // all basic types should have mappings now. what remains is to define the fields of
    // structs and unions to JB so that fields will be mapped to appropriate offsets
    // in this process, any inner structs/unions are inlined into containing struct

    // Third pass: revisit all Structs and Unions to iterate over field types to map them
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        if (type->isStruct()) {
            StructType *sType = static_cast<StructType *>(type);
            char * structName = findOrCreateString(sType->name());
            mapStructFields(typesJB, sType, structName, String(""), 0);
            _mb->typeDictionary()->CloseStruct(structName, sType->size()/8);
        }
        else if (type->isUnion()) {
            UnionType *uType = static_cast<UnionType *>(type);
            char * structName = findOrCreateString(uType->name());
            mapStructFields(typesJB, uType, structName, String(""), 0);
            _mb->typeDictionary()->CloseStruct(structName, uType->size()/8);
        }
    }

    // All types should be represented in the JB layer now, and mapTypes should be set up for every
    // type in TypeDictionary
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        assert(type->isField() || mapType(type) != NULL);
    }

    _methodBuilders[_comp->id()] = _mb;
    storeBuilder(fb, _mb);

    _comp->constructJBFunction(this);
}

FunctionBuilder *
JBCodeGenerator::transformFunctionBuilder(FunctionBuilder *fb) {
    TextLogger *lgr = _fb->logger(traceEnabled());
    if (lgr) lgr->indent() << "JBCodeGenerator transformFunctionBuilder F" << fb->id() << lgr->endl();
    if (lgr) lgr->indentIn();
    return NULL;
}

void
JBCodeGenerator::ConstInt8(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt8(literal->value<int8_t>()));
}

void
JBCodeGenerator::ConstInt16(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt16(literal->value<int16_t>()));
}

void
JBCodeGenerator::ConstInt32(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt32(literal->value<int32_t>()));
}

void
JBCodeGenerator::ConstInt64(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt64(literal->value<int64_t>()));
}

void
JBCodeGenerator::ConstFloat32(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstFloat(literal->value<float>()));
}

void
JBCodeGenerator::ConstFloat64(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstDouble(literal->value<double>()));
}

void
JBCodeGenerator::ConstAddress(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstAddress(literal->value<void *>()));
}

void
JBCodeGenerator::Return(Location *loc, Builder *b, Value *value) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (value)
        omr_b->Return(mapValue(value));
    else
        omr_b->Return();
}

{
   Builder * b = op->parent();
   TR::IlBuilder *omr_b = mapBuilder(b);
   omr_b->setBCIndex(op->location()->bcIndex())->SetCurrentIlGenerator();
   switch (op->action())
      {
      case aCoercePointer :
         {
         TR::IlValue *object = mapValue(op->operand());
         storeValue(op->result(), mapValue(op->operand()));
         }
         break;

      case aAdd :
         storeValue(op->result(), omr_b->Add(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aSub :
         storeValue(op->result(), omr_b->Sub(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aMul :
         storeValue(op->result(), omr_b->Mul(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aLoad :
         {
         Symbol *sym = op->symbol();
         if (sym->isFunction())
            {
            FunctionSymbol *fnSym = static_cast<FunctionSymbol *>(sym);
            storeValue(op->result(), omr_b->ConstAddress(reinterpret_cast<void *>(fnSym->entryPoint())));
            }
         else
            storeValue(op->result(), omr_b->Load(findOrCreateString(sym->name())));
         }
         break;

      case aLoadAt :
         {
         Value *v = op->operand();
         TR::IlValue *jb_v = mapValue(v);
         Type *t = op->type();
         TR::IlType *jb_t = mapType(op->type());
         Value *r = op->result();
         storeValue(r, omr_b->LoadAt(jb_t, jb_v));
         }
         break;

      case aLoadIndirect :
         {
         LoadIndirect *liOp = static_cast<LoadIndirect *>(op);
         FieldType *fieldType = liOp->getFieldType();
         const char *structName = findOrCreateString(fieldType->owningStruct()->name());
         const char *fieldName = findOrCreateString(fieldType->name());
         TR::IlValue *object = mapValue(op->operand());
         assert(object);
         storeValue(op->result(), omr_b->LoadIndirect(structName, fieldName, mapValue(op->operand())));
         }
         break;

      case aStore :
         omr_b->Store(findOrCreateString(op->symbol()->name()), mapValue(op->operand()));
         break;

      case aStoreAt :
         omr_b->StoreAt(mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aStoreIndirect :
         {
         StoreIndirect *siOp = static_cast<StoreIndirect *>(op);
         FieldType *fieldType = siOp->getFieldType();
         const char *structName = findOrCreateString(fieldType->owningStruct()->name());
         const char *fieldName = findOrCreateString(fieldType->name());
         TR::IlValue *object = mapValue(op->operand(0));
         assert(object);
         TR::IlValue *value = mapValue(op->operand(1));
         assert(value);
         omr_b->StoreIndirect(structName, fieldName, object, value);
         }
         break;

      case aIndexAt :
         storeValue(op->result(), omr_b->IndexAt(mapType(op->type()), mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aCall :
         {
         Call *callOp = static_cast<Call *>(op);
         FunctionType *fType = static_cast<FunctionType *>(callOp->function()->type());
         TR::IlValue *args[op->numOperands()];
         args[0] = mapValue(callOp->function());
         for (int32_t a=1;a < op->numOperands();a++)
            args[a] = mapValue(callOp->operand(a));

         if (op->result() != NULL)
            storeValue(op->result(), omr_b->ComputedCall(fType->name().c_str(), callOp->numOperands(), args));
         else
            omr_b->ComputedCall(fType->name().c_str(), callOp->numOperands(), args);
         }
         break;

      case aAppendBuilder :
         omr_b->AppendBuilder(mapBuilder(op->builder()));
         break;

      case aGoto :
         omr_b->Goto(mapBuilder(op->builder()));
         break;

      case aIfCmpGreaterThan :
         omr_b->IfCmpGreaterThan(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpLessThan :
         omr_b->IfCmpLessThan(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpGreaterOrEqual :
         omr_b->IfCmpGreaterOrEqual(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpLessOrEqual :
         omr_b->IfCmpLessOrEqual(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfThenElse :
         {
         TR::IlBuilder *omr_thenB = mapBuilder(op->builder(0));
         TR::IlBuilder *omr_elseB = mapBuilder(op->builder(1));
         omr_b->IfThenElse(&omr_thenB, &omr_elseB, mapValue(op->operand()));
         break;
         }

      case aForLoop :
         {
         TR::IlBuilder *omr_body = mapBuilder(op->builder(0));
         TR::IlBuilder *omr_break = mapBuilder(op->builder(1));
         TR::IlBuilder *omr_continue = mapBuilder(op->builder(2));
         omr_b->ForLoop(static_cast<bool>(op->literal()->getInt8()),
                        findOrCreateString(op->symbol()->name()),
                        &omr_body,
                        &omr_break,
                        &omr_continue,
                        mapValue(op->operand(0)),
                        mapValue(op->operand(1)),
                        mapValue(op->operand(2)));
         break;
         }

      case aSwitch :
         {
         TR::IlBuilder::JBCase *cases[op->numCases()];
         int32_t cNum = 0;
         for (auto cIt = op->CasesBegin(); cIt != op->CasesEnd(); cIt++)
            cases[cNum++] = MAP_CASE(omr_b, *cIt);
         TR::IlBuilder *omr_defaultTarget = mapBuilder(op->builder());
         omr_b->Switch(mapValue(op->operand()), &omr_defaultTarget, op->numCases(), cases);
         break;
         }

      case aCreateLocalArray :
         storeValue(op->result(), omr_b->CreateLocalArray(op->literal(0)->getInt32(), mapType(op->type())));
         break;

      case aCreateLocalStruct :
         storeValue(op->result(), omr_b->CreateLocalStruct(mapType(op->type())));
         break;

      default :
         assert(0); // unhandled action!!
         break;
      }

   return NULL;
   }

FunctionBuilder *
JBCodeGenerator::transformFunctionBuilderAtEnd(FunctionBuilder * fb)
   {
   TextLogger *lgr = fb->logger(traceEnabled());
   if (lgr) lgr->indentOut();
   printAllMaps();
   return NULL;
   }
#endif

} // namespace jbgen
} // namespace JB2
} // namespace OMR

