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
#include "LiteralDictionary.hpp"
#include "Mapper.hpp"
#include "Operation.hpp"
#include "OperationReplacer.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "TypeReplacer.hpp"
#include "TypeReplacerAddon.hpp"
#include "Value.hpp"

using namespace OMR::JB2;

INIT_JBALLOC_ON(TypeReplacer, TypeReplacement)
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
    , _literalsTransformed(false)
    , _symbolsTransformed(false)
    , _typesTransformed(false)
    , _mappedBuildersSize(0)
    , _mappedLiteralsSize(0)
    , _mappedOperandsSize(0)
    , _mappedResultsSize(0)
    , _mappedSymbolsSize(0)
    , _mappedTypesSize(0) {

}

TypeReplacer::~TypeReplacer() {

}

TypeReplacer *
TypeReplacer::replace(const Type *oldType, const Type *newType) {
    assert(newType != oldType);
    _typesToReplace[oldType->id()] = newType;
    removeType(oldType);
    return this;
}

TypeReplacer *
TypeReplacer::explode(const Type *type) {
    assert(type->layout() != NULL);
    assert(type->layout()->size() == type->size());
    _typesToExplode.insert(type->id());
    removeType(type);
    return this;
}

LiteralMapper *
TypeReplacer::literalMapper(Literal *literal) const {
    auto it = _literalMappers.find(literal->id());
    assert(it != _literalMappers.end());
    return it->second;
}

SymbolMapper *
TypeReplacer::symbolMapper(Symbol *sym) const {
    auto it = _symbolMappers.find(sym->id());
    assert(it != _symbolMappers.end());
    return it->second;
}

TypeMapper *
TypeReplacer::typeMapper(const Type *type) const {
    auto it = _typeMappers.find(type->id());
    if (it == _typeMappers.end())
        return NULL;
    return it->second;
}

// for Types
const Type *
TypeReplacer::singleMappedType(const Type *type) {
    TypeMapper *m = typeMapper(type);
    if (m == NULL)
        return type;

    assert(m->size() == 1);
    return m->next();
}

void
TypeReplacer::recordMapper(const Type *type, TypeMapper *m) {
    _examinedType.insert(type->id());
    _typeMappers[type->id()] = m;
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) {
        type->log(*lgr, true);
        lgr->indent() << "Mapped to:" << lgr->endl();
        LOG_INDENT_REGION(lgr) {
            m->start();
            for (uint32_t i=0;i < m->size();i++) {
                const Type *newType = m->next();
                lgr->indent() << i << " : " << " name " << m->name() << " offset " << m->offset() << "\t";
                newType->log(*lgr, true);
            }
        }
        LOG_OUTDENT
        (*lgr) << lgr->endl();
    }
}
 
void
TypeReplacer::recordOriginalType(const Type *type) {
    _examinedType.insert(type->id());
    TextLogger *lgr = _comp->logger(traceEnabled());
    LOG_INDENT_REGION(lgr) {
        if (_typeMappers.find(type->id()) == _typeMappers.end()) {
            Allocator *mem = _comp->mem();
            TypeMapper *m = new (mem) TypeMapper(mem, type);
            recordMapper(type, m);
        }
        assert(_modifiedType.find(type->id()) == _modifiedType.end());
    }
    LOG_OUTDENT
}

// for Symbols
void
TypeReplacer::recordMapper(Symbol *sym, SymbolMapper *m) {
    _symbolMappers[sym->id()] = m;
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) {
        sym->log(*lgr, true);
        lgr->indent() << "Mapped to:" << lgr->endl();
        LOG_INDENT_REGION(lgr) {
            m->start();
            for (uint32_t i=0;i < m->size();i++) {
                const Symbol *newSym = m->next();
                lgr->indent() << i << " : " << " offset " << m->offset() << "\t";
                newSym->log(*lgr, true);
            }
        }
        LOG_OUTDENT
        (*lgr) << lgr->endl();
    }
}

void
TypeReplacer::recordOriginalSymbol(Symbol *sym) {
    TextLogger *lgr = _comp->logger(traceEnabled());
    LOG_INDENT_REGION(lgr) {
        if (lgr) lgr->indent() << "Symbol s" << sym->id() << " unchanged" << lgr->endl();
        if (_symbolMappers.find(sym->id()) == _symbolMappers.end()) {
            Allocator *mem = _comp->mem();
            SymbolMapper *m = new (mem) SymbolMapper(mem, sym);
            recordMapper(sym, m);
        }
    }
    LOG_OUTDENT
}

// for Literals
void
TypeReplacer::recordMapper(Literal *lit, LiteralMapper *m) {
    _literalMappers[lit->id()] = m;
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) {
        lgr->indent() << "Mapped to:" << lgr->endl();
        LOG_INDENT_REGION(lgr) {
            m->start();
            for (uint32_t i=0;i < m->size();i++) {
                const Literal *newLit = m->next();
                lgr->indent() << i << " : " << " offset " << m->offset() << "\t";
                newLit->log(*lgr, true);
            }
        }
        LOG_OUTDENT
        (*lgr) << lgr->endl();
    }
}

void
TypeReplacer::recordOriginalLiteral(Literal *lit) {
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) (*lgr) << " unmodified" << lgr->endl();
    if (_literalMappers.find(lit->id()) == _literalMappers.end()) {
        Allocator *mem = _comp->mem();
        LiteralMapper *m = new (mem) LiteralMapper(mem, lit);
        recordMapper(lit, m);
    }
}

const Type *
TypeReplacer::mappedLayout(const Type *t) {
    const Type *layout = t->layout();
    assert(layout);
    if (_modifiedType.find(layout->id()) != _modifiedType.end()) {
        layout = singleMappedType(layout);
    }
 
    return layout;
}

const Type *
TypeReplacer::replacedType(const Type *type) {
    const Type *replacedType = NULL;
    if (_explodedType.find(type->id()) != _explodedType.end())
        replacedType = mappedLayout(type);
    else
        replacedType = singleMappedType(type);
    return replacedType;
}

const Type *
TypeReplacer::transform(const Type *type) {
    if (isReplacedType(type))
        return replacedType(type);

    if (isTypeToExplode(type)) {
         transformExplodedType(type);
         return NULL;
    }

    return type->replace(this);
}

void
TypeReplacer::transformExplodedType(const Type *type) {
    const Type *layout = type->layout();
    assert(layout);

    Allocator *mem = _comp->mem();
    TypeMapper *m = new (mem) TypeMapper(mem);
    layout->explodeAsLayout(this, 0, m);

    _explodedType.insert(type->id());
    _modifiedType.insert(type->id());
    recordMapper(type, m);
}


void
TypeReplacer::transformLiteralIfNeeded(Literal *literal) {
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) lgr->indent() << "Literal lv" << literal->id();

    const Type *type = literal->type();
    TypeMapper *tm = typeMapper(type);
    if (tm == NULL || tm->size() == 1 && tm->current() == type) {
        recordOriginalLiteral(literal);
        return;
    }

    if (lgr) (*lgr) << " modified" << lgr->endl();

    Allocator *mem = _comp->mem();
    LiteralMapper *m = new (mem) LiteralMapper(mem, lgr);

    // need to explode Literal as many times as there are types to map
    tm->start();
    for (uint32_t i=0;i < tm->size();i++) {
        Literal *explodedLiteral = literal->explode(tm)->refine<Literal>();
        m->add(explodedLiteral);
        recordOriginalLiteral(explodedLiteral);
    }
    recordMapper(literal, m);
    removeLiteral(literal);
}

void
TypeReplacer::transformSymbolIfNeeded(Symbol *sym)
{
    assert(_symbolMappers.find(sym->id()) == _symbolMappers.end());

    TextLogger *lgr = _comp->logger(traceEnabled());
    const Type *type = sym->type();
    TypeMapper *tm = typeMapper(type);
    if (tm->size() == 1 && tm->current() == type) {
        recordOriginalSymbol(sym);
        return;
    }

    Allocator *mem = _comp->mem();
    SymbolMapper *m = new (mem) SymbolMapper(mem, lgr);

    // need to explode Symbol as many times as there are types to map
    tm->start();
    for (uint32_t i=0;i < tm->size();i++) {
        //m->add(sym->explode(tm)->refine<Symbol>());
    }
    recordMapper(sym, m);
}

void
TypeReplacer::transformTypeIfNeeded(const Type *type) {
    TextLogger *lgr = _comp->logger(traceEnabled());
    //if (lgr) type->logType(*lgr);

    if (_examinedType.find(type->id()) != _examinedType.end())
        return;

    _examinedType.insert(type->id());

    const Type *newType = transform(type);
    if (newType != type) {
        _examinedType.insert(type->id());
        _modifiedType.insert(type->id());

        // if type was exploded, newType == NULL and the mapper has already been recorded
        if (newType != NULL) {
            _typesToReplace[type->id()] = newType;

            Allocator *mem = _comp->mem();
            TypeMapper *mapper = new (mem) TypeMapper(mem, newType, lgr);
            recordMapper(type, mapper);
        }
    }
}

void
TypeReplacer::transformTypes(TypeDictionary *dict)
   {
   TextLogger *lgr = _comp->logger(traceEnabled());
   if (lgr)
      {
      lgr->indent() << "TypeReplacer::transformTypes " << dict << lgr->endl();
      lgr->indent() << "Original types:" << lgr->endl();
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
               lgr->indent() << type << " -> " << it2->second << lgr->endl();
            }
         }
      LOG_OUTDENT
      *(lgr) << lgr->endl();
      lgr->indent() << "Transforming now:" << lgr->endl();
      }

   // just to make sure and in case someone calls it twice?
   _examinedType.clear();
   _modifiedType.clear();
   _explodedType.clear();

   LOG_INDENT_REGION(lgr)
      {
      for (auto it = dict->iterator(true, false, true); it.hasItem(); it++)
         {
         const Type *type = it.item();
         transformTypeIfNeeded(type);
         }
      }
   LOG_OUTDENT

   _typesTransformed = true;

   if (lgr) {
       (*lgr) << lgr->endl();
       lgr->indent() << "Transformed types:" << lgr->endl();
       dict->log(*lgr);
       (*lgr) << lgr->endl();

      (*lgr) << lgr->endl();
      lgr->indent() << "Full list of types to replace:" << lgr->endl();
      LOG_INDENT_REGION(lgr)
         {
         for (auto it = dict->iterator();it.hasItem();it++)
            {
            const Type *type = it.item();
            auto it2 = _typesToReplace.find(type->id());
            if (it2 != _typesToReplace.end())
               lgr->indent() << "Replace " << type << " with " << it2->second << lgr->endl();
            }
         }
      LOG_OUTDENT
      *(lgr) << lgr->endl();

      lgr->indent() << "Types to remove in final step:" << lgr->endl();
      LOG_INDENT_REGION(lgr) {
         for (auto it = dict->iterator();it.hasItem();it++) {
            const Type *type = it.item();
            if (_typesToRemove.find(type->id()) != _typesToRemove.end())
               lgr->indent() << type << lgr->endl();
            }
         }
      LOG_OUTDENT
      (*lgr) << lgr->endl();
      }
   }

void
TypeReplacer::transformLiterals(LiteralDictionary *dict)
{
   TextLogger *lgr = _comp->logger(traceEnabled());
   if (lgr)
      {
      lgr->indent() << "TypeReplacer::transformLiterals " << dict << lgr->endl();
      lgr->indent() << "Original literals:" << lgr->endl();
      dict->log(*lgr);
      (*lgr) << lgr->endl();
      lgr->indent() << "Transforming now:" << lgr->endl();
      }

   LOG_INDENT_REGION(lgr)
      {
      for (auto it = dict->iterator(true, false, true); it.hasItem(); it++)
         {
         Literal *literal = it.item();
         transformLiteralIfNeeded(literal);
         }
      }
   LOG_OUTDENT

    if (lgr) {
        lgr->indent() << lgr->endl() << "Transformed literals:" << lgr->endl();
        dict->log(*lgr);
        (*lgr) << lgr->endl();

        lgr->indent() << "Literals to remove in final step:" << lgr->endl();
        LOG_INDENT_REGION(lgr) {
           for (auto it = dict->iterator();it.hasItem();it++) {
              Literal *lv = it.item();
              if (_literalsToRemove.find(lv->id()) != _literalsToRemove.end())
                 lgr->indent() << lv << lgr->endl();
               }
           }
        LOG_OUTDENT
        (*lgr) << lgr->endl();
    }

   _literalsTransformed = true;
}

void
TypeReplacer::transformSymbols(SymbolDictionary *dict)
{
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr) {
        lgr->indent() << "TypeReplacer::transformSymbols " << dict << lgr->endl();
        lgr->indent() << "Original symbols:" << lgr->endl();
        dict->log(*lgr);
        (*lgr) << lgr->endl() << lgr->endl();
        lgr->indent() << "Transforming now:" << lgr->endl();
    }

   LOG_INDENT_REGION(lgr)
      {
      for (auto it = dict->iterator(true, false, true); it.hasItem(); it++)
         {
         Symbol *sym = it.item();
         //transformSymbolIfNeeded(sym);
         }
      }
   LOG_OUTDENT

    if (lgr) {
        lgr->indent() << lgr->endl() << "Transformed symbols:" << lgr->endl();
        dict->log(*lgr);
        (*lgr) << lgr->endl();

       lgr->indent() << "Symbols to remove in final step:" << lgr->endl();
       LOG_INDENT_REGION(lgr) {
          for (auto it = dict->iterator();it.hasItem();it++) {
             Symbol *sym = it.item();
             if (_symbolsToRemove.find(sym->id()) != _symbolsToRemove.end())
                lgr->indent() << sym << lgr->endl();
             }
          }
       LOG_OUTDENT
       (*lgr) << lgr->endl();
    }

   _symbolsTransformed = true;
}

void
TypeReplacer::visitBegin() {
    setTraceEnabled(_comp->config()->traceTypeReplacer());

    if (addons() != NULL) {
        for (auto it = addons()->iterator(); it.hasItem(); it++) {
            Addon *addon = it.item();
            if (addon->isKind<TypeReplacerAddon>()) {
                TypeReplacerAddon *trea = addon->refine<TypeReplacerAddon>();
                trea->collectTypes(_comp->ir());
            }
        }
    }

    if (_typesToExplode.size() == 0 && _typesToReplace.size() == 0) {
        _nothingToDo = true;
        return;
    }
}

void
TypeReplacer::visitPreCompilation(Compilation * comp) {
    TextLogger *lgr = comp->logger(traceEnabled());
    if (lgr) lgr->indent() << "TypeReplacer::visitPreCompilation F" << comp->id() << lgr->endl();
 
    transformTypes(comp->ir()->typedict());
    transformLiterals(comp->ir()->litdict());
    transformSymbols(comp->ir()->symdict());

    if (lgr) lgr->indent() << "Ask Compilation object to replace Types, which may (re)create symbols:" << lgr->endl();
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

Builder *
TypeReplacer::transformOperation(Operation * op) {
    TextLogger *lgr = comp()->logger(traceEnabled());
    TypeDictionary *dict = comp()->ir()->typedict();
    Builder *transformation = NULL;

    if (lgr) lgr->indent() << "Operation " << op << lgr->endl();
    LOG_INDENT_REGION(lgr) {
        if (lgr) op->log(lgr->indent());
        if (lgr) lgr->indent() << "Examining components for replacement or exploding:" << lgr->endl();

        uint32_t numMaps = 0;
        bool cloneNeeded = false;

        Allocator *mem = comp()->passMem();
        OperationReplacer r(op, mem);

        // fill in appropriate mappers based on this operation's operand Values
        for (uint32_t o=0;o < op->numOperands();o++) {
            Value *v = op->operand(o);
            auto it = _valueMappers.find(v->id());
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
            if (_modifiedType.find(lv->type()->id()) != _modifiedType.end())
                cloneNeeded = true;

            auto it = _literalMappers.find(lv->id());
            assert(it != _literalMappers.end());
            LiteralMapper *literalMapper = it->second;
            r.setLiteralMapper(literalMapper, l);
            literalMapper->start();

            if (literalMapper->size() != 1
                || (literalMapper->current()->type() != NULL && literalMapper->current()->type()->id() != lv->type()->id()))
                cloneNeeded = true;

            uint32_t sz = literalMapper->size();
            if (sz > numMaps)
                numMaps = sz;
        }

        // fill in appropriate mappers for this operations Symbols
        for (uint32_t s=0;s < op->numSymbols();s++) {
            Symbol *sym = op->symbol(s);
            if (_modifiedType.find(sym->type()->id()) != _modifiedType.end())
                cloneNeeded = true;

            // Symbols need a bit more care because new ones may have been created that lack a mapper (yet)
            SymbolMapper *symbolMapper = NULL;
            auto it = _symbolMappers.find(sym->id());
            if (it == _symbolMappers.end()) {
                symbolMapper = new (mem) SymbolMapper(mem, sym);
                _symbolMappers.insert({sym->id(), symbolMapper});
            } else {
                symbolMapper = it->second;
            }
            r.setSymbolMapper(symbolMapper, s);
            symbolMapper->start();

            if (symbolMapper->size() != 1 || symbolMapper->current()->id() != sym->id())
                cloneNeeded = true;

            uint32_t sz = symbolMapper->size();
            if (sz > numMaps)
                numMaps = sz;
        }

        // fill in appropriate mappers for this operation's Types
        for (uint32_t t=0;t < op->numTypes();t++) {
            const Type *type = op->type(t);
            auto it = _typeMappers.find(type->id());
            assert(it != _typeMappers.end());
            TypeMapper *typeMapper = it->second;
            r.setTypeMapper(typeMapper, t);
            typeMapper->start();

            if (typeMapper->size() != 1 || typeMapper->current()->id() != type->id())
                cloneNeeded = true;

            uint32_t sz = typeMapper->size();
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

        // create result mappers and insert as operand mappers but don't fill in yet because that happens in different ways
        for (uint32_t i=0;i < op->numResults();i++) {
            Value *result = op->result(i);
            ValueMapper *resultMapper = new (mem) ValueMapper(mem);
            r.setResultMapper(resultMapper, i);
            _valueMappers.insert(std::make_pair(result->id(), resultMapper));
        }

        if (!cloneNeeded) {
            if (lgr) lgr->indent() << "No clone needed, using original operation result(s) if any" << lgr->endl();

            // just map results to themselves and we're done
            for (uint32_t i=0;i < op->numResults();i++) {
                ValueMapper *resultMapper = r.resultMapper(i);
                Value *result = op->result(i);
                resultMapper->add(result);
            }
            return NULL;
        }

        // otherwise this operation needs to be cloned
        transformation = compiler()->coreExt()->OrphanBuilder(LOC, op->parent());

        if (lgr) lgr->indent() << "Cloning operation" << lgr->endl();
        cloneOperation(transformation, &r, numMaps);
    }
    LOG_OUTDENT
    return transformation;
}

void
TypeReplacer::cloneOperation(Builder *b, OperationReplacer *r, uint32_t numMaps) {
    TextLogger *lgr = _comp->logger(traceEnabled());

    Operation *origOp = r->operation();
    if (origOp->hasExpander() && origOp->expand(b, r))
        return;

    if (addons() != NULL) {
        for (auto it = addons()->iterator(); it.hasItem(); it++) {
            Addon *addon = it.item();
            if (addon->isKind<TypeReplacerAddon>()) {
                TypeReplacerAddon *trea = addon->refine<TypeReplacerAddon>();
                if (trea->hasExpander(origOp)) {
                    if (trea->expand(b, origOp, r))
                        return;
                }
            }
        }
    }

    // store mappings for any results
    for (uint32_t i=0;i < origOp->numResults();i++) {
        ValueMapper *resultMapper = r->resultMapper(i);

        Value *result = origOp->result(i);
        const Type *type = result->type();
        auto it = _typeMappers.find(type->id());
        if (it == _typeMappers.end()) {
            // just use the original result
            resultMapper->add(result);
            continue;
        }

        TypeMapper *typeMapper = it->second;
        typeMapper->start();
        for (uint32_t j=0;j < typeMapper->size();j++) {
            const Type *type = typeMapper->next();
            Value *newResult = type->ext()->createValue(b, type);
            resultMapper->add(newResult);
        }

        uint32_t sz = resultMapper->size();
        if (sz > numMaps)
            numMaps = sz;

    }

    // clone the operation for each exploded value
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
            if (lgr) lgr->indent() << "Removing ";
            TypeID idToRemove = *typeIt;
            const Type *typeToRemove = dict->Lookup(idToRemove);
            if (lgr) typeToRemove->log(*lgr);
            if (lgr) (*lgr) << lgr->endl();
            dict->Remove(typeToRemove);
        }
        if (lgr) (*lgr) << lgr->endl();
    }
    LOG_OUTDENT

    if (lgr) {
        lgr->indent() << "Final Type dictionary:" << lgr->endl();
        _comp->ir()->typedict()->log(*lgr);
        *lgr << lgr->endl();

        lgr->indent() << "Final stage: removing literals (" << _literalsToRemove.size() << " literals registered for removal):" << lgr->endl();
    }

    LOG_INDENT_REGION(lgr) {
        LiteralDictionary *dict = _comp->ir()->litdict();
        for (auto litIt = _literalsToRemove.begin(); litIt != _literalsToRemove.end(); litIt++) {
            if (lgr) lgr->indent() << "Removing ";
            LiteralID idToRemove = *litIt;
            Literal *literalToRemove = dict->Lookup(idToRemove);
            if (lgr) literalToRemove->log(*lgr);
            if (lgr) (*lgr) << lgr->endl();
            dict->Remove(literalToRemove);
        }
        if (lgr) (*lgr) << lgr->endl();
    }
    LOG_OUTDENT

     if (lgr) {
         lgr->indent() << "Final Literal dictionary:" << lgr->endl();
         _comp->ir()->litdict()->log(*lgr);
         *lgr << lgr->endl();

         lgr->indent() << "Final stage: removing symbols (" << _symbolsToRemove.size() << " symbols registered for removal):" << lgr->endl();
     }

    LOG_INDENT_REGION(lgr) {
        SymbolDictionary *dict = _comp->ir()->symdict();
        for (auto symIt = _symbolsToRemove.begin(); symIt != _symbolsToRemove.end(); symIt++) {
            if (lgr) lgr->indent() << "Removing ";
            SymbolID idToRemove = *symIt;
            Symbol *symToRemove = dict->Lookup(idToRemove);
            if (lgr) symToRemove->log(*lgr);
            if (lgr) (*lgr) << lgr->endl();
            dict->Remove(symToRemove);
        }
        if (lgr) (*lgr) << lgr->endl();
    }
    LOG_OUTDENT

     if (lgr) {
         lgr->indent() << "Final Symbol dictionary:" << lgr->endl();
         _comp->ir()->symdict()->log(*lgr);
         *lgr << lgr->endl();
     }
}
