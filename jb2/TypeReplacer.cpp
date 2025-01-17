/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
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
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Config.hpp"
#include "CoreExtension.hpp"
#include "Builder.hpp"
#include "Extension.hpp"
#include "Literal.hpp"
//#include "DynamicType.hpp"
#include "Operation.hpp"
#include "OperationReplacer.hpp"
#include "Symbol.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "TypeReplacer.hpp"
#include "Value.hpp"

using namespace OMR::JB2;

SUBCLASS_KINDSERVICE_IMPL(TypeReplacer,"TypeReplacer",Transformer,Extensible);

// TypeReplacer is responsible for rewriting a Function according to a
// list of types to be replaced (with a corresponding list of replacement types)
// as well as "exploding" a list of types to be replaced by their layout types.
// Typically, user defined types are exploded before they can be generated. For
// example, a user defined Complex type might be exploded into two Double values.
// Upon creation, one can describe the types to be replaced/exploded by calling
// replaceType/explodeType on the TypeReplacer:
// e.g.
//    TypeReplacer *tr = new TypeReplacer(comp)
//                       ->explodeType(complexExtension->Complex)
//                       ->explodeType(cartesianExtension->Point3D)
//                       ->replaceType(baseExtension->Double, baseExtension->Float)
//                       ->replaceType(myExtension->T, baseExtension->Int32);
//
// TypeReplacer uses Mappers to perform explosion and replacement: a Mapper provides
// a way to repetitively iterate through a Type's replacement Type or through the
// Types that result from exploding a Type's layout. By creating a specific Mapper
// for each element (operand, literal value, type, symbol) of an Operation, the
// Operation can be transformed into as many replacement Operations as needed by
// simply cloning it repeatedly from the Mappers built from the original Operation's
// elements, with each clone asking for the "next" element from each corresponding
// Mapper. Non-exploded elements will simply keep providing the same (possibly replaced)
// element type for each clone, whereas exploded elements will provide a different
// element for each clone of the Operation. This simplistic approach may not work
// for all kinds of Operations, so Operations can override this simple expansion
// technique where needed.
//
// To save space and time, TypeReplacer caches all Mapper objects created so they
// can be reused by later replacing/exploding similar kinds of elements. It begins by
// walking each Type to see if it references a Type that needs to be "transformed"
// (either replaced or exploded or built directly or indirectly from such a Type).
// The Function's signature elements (return value, parameters) are then processed,
// followed by an iteration over all the Operations in the Function.
//
// When an element (Value, Literal, Type, Symbol) is first encountered while
// visiting the operations of the Function, that element is transformed to
// an appropriate Mapper object (ValueMapper, LiteralMapper, TypeMapper, SymbolMapper)
// and then recorded in a std::map from the original element to its Mapper object.
// The collection of Mapper objects is used to create an OperationReplacer object
// created for the original Operation. This OperationReplacer iterates over the
// Types returned by the Mappers to create new Operations. As these new Operations are
// created, if they produce result Values, these Values are added by the OperationReplacer
// to ValueMappers. These ValueMappers are then recorded into the ValueMapper cache so
// that subsequent uses of the original result Value will find thes result ValueMapper
// objects.
//
// A Function's signature (set of parameters) are also remapped in this way, so the
// resulting Function may take more parameters than the original Function (if any
// Types were exploded, for example). The same is true for all LocalSymbols (they
// may be exploded into multiple LocalSymbols). Note that Type's cannot be exploded
// if they are referenced via a PointerType, so TypeReplacer replaces PointerTo(explodedType)
// with PointerTo(explodedType->layout())
//
// A Type that "explodes" will be replaced with its layout fields where ever
// it is used. Similarly, structs that have a field defined as an exploded Type
// will have their fields exploded into the containing struct. In this case, however,
// a duplicate field of the layout type itself will be created at the offset of the
// original field, so that the address of the beginning of the layout struct can be
// easily computed. Parameters of an exploded Type will be replaced with multiple
// parameters from the fields of the layout struct (TODO: is this anything like what
// the ABI does?). LocalSymbols are similarly expanded. Unions with a field of an
// exploded type will be replaced with the layout struct type, all located at the
// same offset.
//
// Note that Types that do not directly refer to exploded or replaced Types may still be
// changed by this pass. For example, PointerTo(PointerTo(explodedType)) must be changed
// because PointerTo(explodedType) will be changed to PointerTo(explodedType->layout()).
//


TypeReplacer::TypeReplacer(Allocator *a, Compiler * compiler)
    : Transformer(a, CLASSKIND(TypeReplacer, Extensible), compiler->coreExt(), String(a, "TypeReplacer"))
    , _typesTransformed(false) {

}

TypeReplacer::~TypeReplacer() {

}

TypeReplacer *
TypeReplacer::replace(const Type *oldType, const Type *newType) {
    assert(newType != oldType);
    _typesToReplace[oldType->id()] = newType->id();
    return this;
}

TypeReplacer *
TypeReplacer::explode(const Type *type) {
    assert(type->layout() != NULL);
    assert(type->layout()->size() == type->size());
    _typesToExplode.insert(type->id());
    return this;
}

void
TypeReplacer::recordMapper(const Type *type, TypeMapper *mapper) {
    _examinedType.insert(type);
    _typeMappers[type] = mapper;
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) {
        lgr->indent() << "type t" << type->id() << " mapper registered:" << lgr->endl();
        LOG_INDENT_REGION(lgr) {
            for (uint32_t i=0;i < mapper->size();i++) {
                const Type *newType = mapper->current();
                if (lgr) lgr->indent() << i << " : " << "\"" << mapper->name() << "\"" << " offset " << mapper->offset() << " : ";
                if (lgr) newType->logType(*lgr, false);
                mapper->next();
            }
        }
        LOG_OUTDENT
    }
}
 
TypeMapper *
TypeReplacer::mapperForType(const Type *type) const {
    auto it = _typeMappers.find(type);
    assert(it != _typeMappers.end());
    return it->second;
}

void
TypeReplacer::recordOriginalType(const Type *type) {
    _examinedType.insert(type);
    TextLogger *lgr = _comp->logger(traceEnabled());
    LOG_INDENT_REGION(lgr) {
        if (lgr) lgr->indent() << "type t" << type->id() << " unchanged" << lgr->endl();
        if (_typeMappers.find(type) == _typeMappers.end()) {
            Allocator *mem = _comp->mem();
            TypeMapper *m = new (mem) TypeMapper(mem, type);
            recordMapper(type, m);
        }
        assert(_modifiedType.find(type) == _modifiedType.end());
    }
    LOG_OUTDENT
}

void
TypeReplacer::recordSymbolMapper(Symbol *s, SymbolMapper *m) {
    _symbolMappers[s] = m;
}

const Type *
TypeReplacer::singleMappedType(const Type *type) {
    auto it = _typeMappers.find(type);
    assert(it != _typeMappers.end());
    TypeMapper *m = it->second;
    assert(m->size() == 1); // should map only to a Struct
    return m->next();
}

const Type *
TypeReplacer::mappedLayout(const Type *t) {
    const Type *layout = t->layout();
    assert(layout);
    if (_modifiedType.find(layout) != _modifiedType.end()) {
        layout = singleMappedType(layout);
    }
 
    return layout;
}

const Type *
TypeReplacer::replacedType(const Type *type) {
    const Type *replacedType = NULL;
    if (_explodedType.find(type) != _explodedType.end())
        replacedType = mappedLayout(type);
    else
        replacedType = singleMappedType(type);
    return replacedType;
}

const Type *
TypeReplacer::transform(const Type *type) {
    if (isReplacedType(type))
        return replacedType(type);

    #if 0
    else if (isExploded(type)) {
        assert(type->canBeLayout());
        return type->explodeAsLayout(this, 0);
    }
    #endif

    assert(!isExploded(type));
    return singleMappedType(type);
}

void
TypeReplacer::transformExplodedType(const Type *type) {
    const Type *layout = type->layout();
    assert(layout);

    Allocator *mem = _comp->mem();
    TypeMapper *m = new (mem) TypeMapper(mem);
    layout->explodeAsLayout(this, 0, m);

    _explodedType.insert(type);
    recordMapper(type, m);
    _typesToRemove.insert(type);

    // also explode the layout type itself, in case it has inner exploded types
    transformTypeIfNeeded(layout);

     _modifiedType.insert(type);
     _explodedType.insert(type);
}


#if 0
//field type
void
TypeReplacer::explodeLayoutTypes(TypeDictionary *dict, const StructType *layout, size_t baseOffset, TypeMapper *m) {
    for (auto fIt = layout->FieldsBegin(); fIt != layout->FieldsEnd(); fIt++) {
        const FieldType *fType = fIt->second;
        const Type *t = fType->type();
        transformTypeIfNeeded(t);
 
        size_t fieldOffset = baseOffset + fType->offset();
        if (_typesToExplode.find(t->id()) != _typesToExplode.end()) {
            const StructType *innerLayout = t->layout();
            assert(innerLayout);
            explodeLayoutTypes(dict, innerLayout, fieldOffset, m);
        }
        else {
            const Type *mappedType = singleMappedType(t);
            Literal *name = fType->fieldName();
            String fieldName = name->getString();
            if (name->kind() == T_typename)
                fieldName = mappedType->name();
            m->add(mappedType, fieldName, fieldOffset);
        }
    }
}
 

void
TypeReplacer::transformType(const Type *type) {

}
#endif

void
TypeReplacer::transformTypeIfNeeded(const Type *type) {
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) type->logType(*lgr);

    if (_examinedType.find(type) != _examinedType.end())
        return;

    _examinedType.insert(type);
    _modifiedType.erase(type);
    _explodedType.erase(type);
    _typesToRemove.insert(type);

    if (isExploded(type)) {
         transformExplodedType(type);
         return;
    }

    const Type *newType = transform(type);
    if (newType) {
        _examinedType.insert(newType);
        _modifiedType.insert(type);

        Allocator *mem = _comp->mem();
        TypeMapper *mapper = new (mem) TypeMapper(mem, newType);
        recordMapper(type, mapper);
        recordOriginalType(newType);
    }
}

    #if 0
   LOG_INDENT_REGION(lgr)
      {
      else if (type->isField())
         {
         FieldType *fType = static_cast<FieldType *>(type);
         Type *fieldType = fType->type();
         LOG_INDENT_REGION(lgr)
            {
            if (lgr) lgr->indent() << "FieldType " << fType->name() << " type t" << fieldType->id() << lgr->endl();

            transformTypeIfNeeded(fieldType);
            if (_modifiedType.find(fieldType) == _modifiedType.end())
               recordOriginalType(type); // ensure recorded
            else
               {
               // modified types are handled via struct/union types so just ignore here
               if (lgr) lgr->indent() << "modified field to be handled when struct is transformed" << lgr->endl();
               }
            }
         LOG_OUTDENT
         }

      else if (type->isStruct() || type->isUnion())
         {
         StructType *sType = static_cast<StructType *>(type);
         bool transform = false;

         LOG_INDENT_REGION(lgr)
            {
            if (lgr) lgr->indent() << "Struct/UnionType" << lgr->endl();

            for (auto fIt = sType->FieldsBegin(); fIt != sType->FieldsEnd(); fIt++)
               {
               FieldType *fType = fIt->second;
               if (lgr) lgr->indent() << "Examining field " << fType << " ( " << fType->name() << " )" << lgr->endl();

               transformTypeIfNeeded(fType);
               if (_modifiedType.find(fType->type()) != _modifiedType.end())
                  transform = true;
               }

            if (transform)
               transformStructType(dict, sType);
            else
               recordOriginalType(type);
            }
         LOG_OUTDENT
         }

      // for functions, if return type or any parameter type needs to be changed (doesn't matter to what, here)
      //   then construct a new function type with new types
      else if (type->isFunction())
         {
         FunctionType *fnType = static_cast<FunctionType *>(type);
         bool transform = false;

         LOG_INDENT_REGION(lgr)
            {
            if (lgr) lgr->indent() << "FunctionType" << lgr->endl();

            Type *returnType = fnType->returnType();
            transformTypeIfNeeded(returnType);

            if (_modifiedType.find(returnType) != _modifiedType.end())
               transform = true;

            for (int32_t p=0;p < fnType->numParms();p++)
               {
               Type *pType = fnType->parmType(p);
               transformTypeIfNeeded(pType);

               if (_modifiedType.find(pType) != _modifiedType.end())
                  transform = true;
               }

            if (transform)
               transformFunctionType(dict, fnType);
            else
               recordOriginalType(type);
            }
         LOG_OUTDENT
         }

      //
      // User composite type handling here
      // BEGIN {

      // } END
      // User composite type handling here
      //

      else if (_typesToExplode.find(type->id()) != _typesToExplode.end())
         {
         transformExplodedType(dict, type);
         _modifiedType.insert(type);
         _explodedType.insert(type);
         }
      else
         {
         auto it = _typesToReplace.find(typeID);
         if (it != _typesToReplace.end())
            {
            Type *typeToReplace = dict->Lookup(it->second);
            Allocator *mem = _comp->mem();
            TypeMapper *m = new (mem) TypeMapper(mem, typeToReplace);
            recordMapper(type, m);
            _modifiedType.insert(type);
            }
         else
            recordOriginalType(type);
         }
      }
      LOG_OUTDENT
   #endif

void
TypeReplacer::transformTypes(TypeDictionary *dict)
   {
   TextLogger *lgr = _comp->logger(traceEnabled());
   if (lgr)
      {
      lgr->indent() << "TypeReplacer::transformTypes " << dict << lgr->endl();
      dict->log(*lgr);
      (*lgr) << lgr->endl();
      lgr->indent() << "Types to explode:" << lgr->endl();
      LOG_INDENT_REGION(lgr)
         {
         for (auto it = dict->iterator();it.hasItem();it++)
            {
            const Type *type = it.item();
            if (_typesToExplode.find(type->id()) != _typesToExplode.end())
               lgr->indent() << type << lgr->endl();
            }
         }
      LOG_OUTDENT

      (*lgr) << lgr->endl();
      lgr->indent() << "Types to replace:" << lgr->endl();
      LOG_INDENT_REGION(lgr)
         {
         for (auto it = dict->iterator();it.hasItem();it++)
            {
            const Type *type = it.item();
            auto it2 = _typesToReplace.find(type->id());
            if (it2 != _typesToReplace.end())
               lgr->indent() << "Replace " << type << " with " << dict->Lookup(it2->second) << lgr->endl();
            }
         }
      LOG_OUTDENT
      lgr->indent() << "Transforming now:" << lgr->endl();
      }

   // just to make sure and in case someone calls it twice?
   _examinedType.clear();
   _modifiedType.clear();
   _explodedType.clear();

   LOG_INDENT_REGION(lgr)
      {
      for (auto it = dict->iterator(); it.hasItem(); it++)
         {
         const Type *type = it.item();
         transformTypeIfNeeded(type);
         }
      }
   LOG_OUTDENT

   if (lgr) lgr->indent() << lgr->endl() << "Transformed dictionary:" << lgr->endl();
   if (lgr) dict->log(*lgr);

   _typesTransformed = true;
   if (lgr)
      {
      lgr->indent() << "Types to remove in final step:" << lgr->endl();
      LOG_INDENT_REGION(lgr)
         {
         for (auto it = dict->iterator();it.hasItem();it++)
            {
            const Type *type = it.item();
            if (_typesToRemove.find(type) != _typesToRemove.end())
               lgr->indent() << type << lgr->endl();
            }
         }
      LOG_OUTDENT
      }
   }

void
TypeReplacer::visitBegin() {
    setTraceEnabled(_comp->config()->traceTypeReplacer());
}

void
TypeReplacer::visitPreCompilation(Compilation * comp) {
    TextLogger *lgr = comp->logger(traceEnabled());
    if (lgr) lgr->indent() << "TypeReplacer::visitPreCompilation F" << comp->id() << lgr->endl();
 
    if (lgr) lgr->indent() << "TypeReplacer::look for new Types:" << lgr->endl();
    LOG_INDENT_REGION(lgr) {
        TypeDictionary *dict = comp->ir()->typedict();
        for (auto it = dict->iterator(); it.hasItem(); it++) {
            const Type *type = it.item();
            transformTypeIfNeeded(type);
        }
    }
    LOG_OUTDENT

    comp->replaceTypes(this);

    Allocator *mem = comp->mem();
    // values in operations will be replaced last, handled by transformOperation
    // first, set up a set of Mappers for those operations to use
    _mappedResultsSize = 1;
    _mappedResults = mem->allocate<ValueMapper *>(1);
    _mappedResults[0] = NULL;

    _mappedOperandsSize = 2;
    _mappedOperands = mem->allocate<ValueMapper *>(2);
    _mappedOperands[0] = NULL;
    _mappedOperands[1] = NULL;

    _mappedTypesSize = 1;
    _mappedTypes = mem->allocate<TypeMapper *>(1);
    _mappedTypes[0] = new (mem) TypeMapper(mem);

    _mappedSymbolsSize = 1;
    _mappedSymbols = mem->allocate<SymbolMapper *>(1);
    _mappedSymbols[0] = new (mem) SymbolMapper(mem);

    _mappedLiteralsSize = 1;
    _mappedLiterals = mem->allocate<LiteralMapper *>(1);
    _mappedLiterals[0] = new (mem) LiteralMapper(mem);

    if (lgr) lgr->indent() << lgr->endl() << "About to transform operations" << lgr->endl() << lgr->endl();
}

void
TypeReplacer::transformLiteral(Literal *lv) {
    LiteralMapper *m = NULL;
    const Type *t = lv->type();
    TypeID typeID = t->id();

    if (_explodedType.find(t) != _explodedType.end())
        m = t->explode(lv);
    else if (_modifiedType.find(t) != _modifiedType.end()) {
        // TODO: m = t->convert(lv, _typeMappers.find(t));
        assert(0);
    }
    else {
        Allocator *mem = _comp->mem();
        m = new (mem) LiteralMapper(mem, lv);
    }

    assert(m);
    _literalMappers[lv] = m;
}

Builder *
TypeReplacer::transformOperation(Operation * op) {
    TextLogger *lgr = comp()->logger(traceEnabled());
    TypeDictionary *dict = comp()->ir()->typedict();
    Builder *b = NULL;

    LOG_INDENT_REGION(lgr) {
        uint32_t numMaps = 0;
        bool cloneNeeded = false;

        Allocator *mem = comp()->passMem();
        OperationReplacer r(mem, op);

        // fill in appropriate mappers based on this operation's operand Values
        for (uint32_t o=0;o < op->numOperands();o++) {
            Value *v = op->operand(o);
            auto it = _valueMappers.find(v);
            assert(it != _valueMappers.end()); // must have been produced earlier
            ValueMapper *valueMapper = it->second;
            r.setOperandMapper(valueMapper, o);
            valueMapper->start();
            if (valueMapper->size() != 1 || valueMapper->current()->id() != v->id())
                cloneNeeded = true;
            uint32_t sz = valueMapper->size();
            if (sz > numMaps)
                numMaps = sz;
        }

        // transform literals as needed and fill in appropriate mappers for this operation's Literal
        for (uint32_t l=0;l < op->numLiterals();l++) {
            Literal *lv = op->literal(l);
            if (_literalMappers.find(lv) == _literalMappers.end())
                transformLiteral(lv);

            auto it = _literalMappers.find(lv);
            assert(it != _literalMappers.end());
            LiteralMapper *literalMapper = it->second;
            r.setLiteralMapper(literalMapper, l);
            literalMapper->start();

            if (literalMapper->size() != 1
                || (literalMapper->current()->type() != NULL && literalMapper->current()->type()->id() != lv->type()->id()))
                cloneNeeded = true;

            uint32_t sz = _mappedLiterals[l]->size();
            if (sz > numMaps)
                numMaps = sz;
        }

        // fill in appropriate mappers for this operations Symbols
        for (uint32_t s=0;s < op->numSymbols();s++) {
            Symbol *sym = op->symbol(s);
            if (_modifiedType.find(sym->type()) != _modifiedType.end())
                cloneNeeded = true;

            auto it = _symbolMappers.find(sym);
            assert(it != _symbolMappers.end());
            SymbolMapper *symbolMapper = it->second;
            r.setSymbolMapper(symbolMapper, s);
            symbolMapper->start();

            if (symbolMapper->size() != 1 || symbolMapper->current()->id() != sym->id())
                cloneNeeded = true;

            uint32_t sz = it->second->size();
            if (sz > numMaps)
                numMaps = sz;
        }

        // fill in appropriate mappers for this operation's Types
        for (uint32_t t=0;t < op->numTypes();t++) {
            const Type *type = op->type(t);
            auto it = _typeMappers.find(type);
            assert(it != _typeMappers.end());
            TypeMapper *typeMapper = it->second;
            r.setTypeMapper(typeMapper, t);
            typeMapper->start();

            if (typeMapper->size() != 1 || typeMapper->current()->id() != type->id())
                cloneNeeded = true;

            uint32_t sz = it->second->size();
            if (sz > numMaps)
                numMaps = sz;
        }

        // no Builder mappings are done at this time, so just create a mapper for each Builder
        //   initialized with the operation's original Builder
        for (uint32_t b=0;b < op->numBuilders();b++) {
            r.setBuilderMapper(new (mem) BuilderMapper(mem, op->builder(b)), b);
            if (numMaps < 1)
                numMaps = 1;
        }

        if (!cloneNeeded) {
            if (lgr) lgr->indent() << "No clone needed, using original operation result(s) if any" << lgr->endl();

            // just map results to themselves and we're done
            for (uint32_t i=0;i < op->numResults();i++) {
                Value *result = op->result(i);
                r.setResultMapper(new (mem) ValueMapper(mem, result), i);
            }
            return NULL;
        }

        // otherwise this operation needs to be cloned
        if (lgr) lgr->indent() << "Cloning operation" << lgr->endl();
        b = compiler()->coreExt()->OrphanBuilder(LOC, op->parent());
        cloneOperation(b, &r, numMaps);

        // store any new result mappings
        for (uint32_t i=0;i < op->numResults();i++) {
            Value *result = op->result(i);
            assert(_valueMappers.find(result) == _valueMappers.end());
            _valueMappers[result] = r.resultMapper(i);
        }
    }
    LOG_OUTDENT
    return b;
}

void
TypeReplacer::cloneOperation(Builder *b, OperationReplacer *r, uint32_t numMaps) {
    TextLogger *lgr = _comp->logger(traceEnabled());

    Operation *origOp = r->operation();
    if (origOp->hasExpander() && origOp->expand(r))
        return;

    #if 0
    // need this with DynamicOperations
    for (auto it=op->OperandsBegin();it != op->OperandsEnd();it++) {
        Value *v = *it;
        Type *t = v->type();
        if (t->isDynamic()) {
            DynamicType *type = static_cast<DynamicType *>(t);
            if (type->expand(&r))
                return;
        }
    }
    #endif

    // otherwise, map the operation generically
    for (uint32_t i=0;i < numMaps;i++)
        r->clone(b);
}

void
TypeReplacer::visitPostCompilation(Compilation * comp)
   {
   finalCleanup();
   }

void
TypeReplacer::finalCleanup() {
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) lgr->indent() << "Final stage: removing types (" << _typesToRemove.size() << " types registered for removal):" << lgr->endl();

    LOG_INDENT_REGION(lgr) {
        TypeDictionary *dict = _comp->ir()->typedict();
        for (auto typeIt = _typesToRemove.begin(); typeIt != _typesToRemove.end(); typeIt++) {
            const Type *typeToRemove = *typeIt;
            #if 0
            if (typeToRemove->isField()) {
                // be careful: make sure owning struct isn't marked for removal
                //             if it is, then we would remove this field type twice
                const FieldType *fType = static_cast<const FieldType *>(typeToRemove);
                if (fType->owningStruct()->owningDictionary() == dict
                    && _typesToRemove.find(fType->owningStruct()) != _typesToRemove.end()) {

                    if (lgr) lgr->indent() << "Ignoring field type inside to-be-removed struct: ";
                    if (lgr) lgr->writeType(typeToRemove);
                    continue;
                }
            }
            #endif
            if (lgr) lgr->indent() << "Removing ";
            if (lgr) typeToRemove->logType(*lgr);
            dict->Remove(typeToRemove);
        }
    }
    LOG_OUTDENT

    if (lgr) lgr->indent() << "Final dictionary:" << lgr->endl();
    if (lgr) _comp->ir()->typedict()->log(*lgr);
}
