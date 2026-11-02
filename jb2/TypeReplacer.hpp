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
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef TYPEREPLACER_INCL
#define TYPEREPLACER_INCL


#include <map>
#include <set>
#include "Transformer.hpp"
#include "Literal.hpp"
#include "Mapper.hpp"
#include "Symbol.hpp"
#include "Type.hpp"

namespace OMR {
namespace JB2 {

class Builder;
class FieldType;
class FunctionType;
class Operation;
class OperationReplacer;
class PointerType;
class StructType;
class Type;

class TypeReplacer : public Transformer {
    JBALLOC_(TypeReplacer)

    friend class Type;

public:
    DYNAMIC_ALLOC_ONLY(TypeReplacer, Compiler * compiler);

    Compilation *comp() const { return _comp; }

    // (NOT YET IMPLEMENTED) all references of oldType will be changed to newType
    // when transform() returns, there will be no remaining references to oldType in operations or
    //   in symbols or literals referenced by operations
    // (though the type itself is not erased from the FunctionBuilder's TypeDictionary)
    TypeReplacer *replace(const Type *oldType, const Type *newType=NULL);

    // type must return a non-NULL layout()
    // when transform() returns, there will be no remaining references to type in operations or
    //   in symbols or literals referenced by operations
    // (though the type itself is not erased from the FunctionBuilder's TypeDictionary)
    TypeReplacer *explode(const Type *type);
    bool isTypeToExplode(const Type *type) const { return _typesToExplode.find(type->id()) != _typesToExplode.end();  }

    void transformTypes(TypeDictionary *dict);
    void transformLiterals(LiteralDictionary *dict);
    void transformSymbols(SymbolDictionary *dict);
    void finalCleanup();

    bool isModified(const Type *type) const { return _modifiedType.find(type->id()) != _modifiedType.end(); }
    bool isExploded(const Type *type) const { return _explodedType.find(type->id()) != _explodedType.end(); }

    bool isReplacedType(const Type *type) const { return _explodedType.find(type->id()) != _explodedType.end(); }
    const Type * replacedType(const Type *type);

    bool isRemovedLiteral(Literal *lv) const { return _literalsToRemove.find(lv->id()) != _literalsToRemove.end(); }
    void removeLiteral(Literal *lv) { _literalsToRemove.insert(lv->id()); }

    bool isRemovedSymbol(Symbol *s) const { return _symbolsToRemove.find(s->id()) != _symbolsToRemove.end(); }
    void removeSymbol(Symbol *s) { _symbolsToRemove.insert(s->id()); }

    bool isRemovedType(const Type *type) const { return _typesToRemove.find(type->id()) != _typesToRemove.end(); }
    void removeType(const Type *type) { _typesToRemove.insert(type->id()); }

    void recordMapper(const Type *type, TypeMapper *mapper);
    void recordMapper(Symbol *sym, SymbolMapper *mapper);
    void recordMapper(Literal *lit, LiteralMapper *mapper);

    void recordOriginalType(const Type *type);
    void recordOriginalLiteral(Literal *lit);
    void recordOriginalSymbol(Symbol *sym);

    TypeMapper *typeMapper(const Type *type) const;
    const Type * singleMappedType(const Type *type);
    const Type * transform(const Type *type);
    void transformTypeIfNeeded(const Type *type);

    LiteralMapper *literalMapper(Literal *literal) const;
    void transformLiteralIfNeeded(Literal *literal);

    SymbolMapper *symbolMapper(Symbol *sym) const;
    void transformSymbolIfNeeded(Symbol *sym);


protected:
    // override Transformer and Visitor functions
    virtual void visitBegin();
    virtual void visitPreCompilation(Compilation * comp);
    virtual Builder * transformOperation(Operation * op);
    virtual void visitPostCompilation(Compilation * comp);

    // overrideable functions
    virtual void cloneOperation(Builder *b, OperationReplacer *r, uint32_t numMaps);

    // helper functions
    const Type * mappedLayout(const Type *t);

    void explodeLayoutTypes(TypeDictionary *dict, const StructType *layout, size_t baseOOffset, TypeMapper *m);
    void transformExplodedType(const Type *type);
    void transformLiteral(Literal *lv);

    // just recording progress
    bool                                _literalsTransformed;
    bool                                _symbolsTransformed;
    bool                                _typesTransformed;

    std::set<TypeID>                    _typesToExplode;
    std::map<TypeID,const Type *>       _typesToReplace;

    std::set<TypeID>                    _typesToRemove;
    std::set<SymbolID>                  _symbolsToRemove;
    std::set<LiteralID>                 _literalsToRemove;

    std::map<LiteralID,LiteralMapper *> _literalMappers;
    std::map<SymbolID,SymbolMapper *>   _symbolMappers;
    std::map<TypeID,TypeMapper *>       _typeMappers;
    std::map<ValueID,ValueMapper *>     _valueMappers;

    std::set<TypeID>                    _explodedType;
    std::set<TypeID>                    _modifiedType;
    std::set<TypeID>                    _examinedType;

    // below are mappers that will be initialized according to the current operation
    size_t _mappedBuildersSize;
    BuilderMapper **_mappedBuilders;

    size_t _mappedLiteralsSize;
    LiteralMapper **_mappedLiterals;

    size_t _mappedOperandsSize;
    ValueMapper **_mappedOperands;

    size_t _mappedResultsSize;
    ValueMapper **_mappedResults;

    size_t _mappedSymbolsSize;
    SymbolMapper **_mappedSymbols;

    size_t _mappedTypesSize;
    TypeMapper **_mappedTypes;

    SUBCLASS_KINDSERVICE_DECL(Extensible, TypeReplacer);
};

} // namespace JB2
} // namespace OMR

#endif // defined(TYPEREPLACER_INCL)
