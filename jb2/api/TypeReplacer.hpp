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
#include <vector>
#include "Transformer.hpp"
#include "Mapper.hpp"
#include "Type.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class FieldType;
class FunctionType;
class Operation;
class OperationReplacer;
class PointerType;
class StructType;
class Type;

class TypeReplacer : public Transformer {
    friend class Type;

public:
    TypeReplacer(Compiler * compiler);

    bool traceEnabled() const { return _traceEnabled; }
    Compilation *comp() const { return _comp; }

    // all references of oldType will be changed to newType
    // when transform() returns, there will be no remaining references to oldType
    // (though the type itself is not erased from the FunctionBuilder's TypeDictionary)
    TypeReplacer *replace(const Type *oldType, const Type *newType=NULL);

    // type must return a non-NULL layout()
    // when transform() returns, there will be no remaining references to type
    // (though the type itself is not erased from the FunctionBuilder's TypeDictionary)
    TypeReplacer *explode(const Type *type);

    void transformTypes(TypeDictionary *dict);
    void finalCleanup();

    bool isModified(const Type *type) const { return _modifiedType.find(type) != _modifiedType.end(); }
    bool isExploded(const Type *type) const { return _explodedType.find(type) != _explodedType.end(); }
    bool isReplacedType(const Type *type) const { return _explodedType.find(type) != _explodedType.end(); }
    const Type * replacedType(const Type *type);
    bool isRemovedType(const Type *type) const { return _typesToRemove.find(type) != _typesToRemove.end(); }
    void removeType(const Type *type) { _typesToRemove.insert(type); }
    void recordMapper(const Type *type, TypeMapper *mapper);
    void recordSymbolMapper(Symbol *sym, SymbolMapper *mapper);
    void recordOriginalType(const Type *type);
    const Type * singleMappedType(const Type *type);
    void transformTypeIfNeeded(const Type *type);
    const Type * transform(const Type *type);

    TypeMapper *mapperForType(const Type *type) const;

protected:
    // override Transformer and Visitor functions
    virtual void visitBegin();
    virtual void visitPreCompilation(Compilation * comp);
    virtual Builder * transformOperation(Operation * op);
    virtual void visitPostCompilation(Compilation * comp);

    // overrideable functions
    virtual void cloneOperation(Builder *b, OperationReplacer *r, int numMaps);

    // helper functions
    const Type * mappedLayout(const Type *t);
    #if 0 // how to handle this?
    #endif

    void explodeLayoutTypes(TypeDictionary *dict, const StructType *layout, size_t baseOOffset, TypeMapper *m);
    void transformExplodedType(const Type *type);
    void transformLiteral(Literal *lv);

    bool                                _typesTransformed;

    std::set<const Type *>              _typesToRemove;

    std::set<TypeID>                    _typesToExplode;
    std::map<TypeID,TypeID>             _typesToReplace;

    std::map<Literal *,LiteralMapper *> _literalMappers;
    std::map<Symbol *,SymbolMapper *>   _symbolMappers;
    std::map<const Type *,TypeMapper *> _typeMappers;
    std::map<Value *,ValueMapper *>     _valueMappers;

    std::set<const Type *>              _explodedType;
    std::set<const Type *>              _modifiedType;
    std::set<const Type *>              _examinedType;

    // below are mappers that will be initialized according to the current operation
    size_t _mappedResultsSize;
    ValueMapper **_mappedResults;

    size_t _mappedOperandsSize;
    ValueMapper **_mappedOperands;

    size_t _mappedSymbolsSize;
    SymbolMapper **_mappedSymbols;

    size_t _mappedLiteralsSize;
    LiteralMapper **_mappedLiterals;

    size_t _mappedTypesSize;
    TypeMapper **_mappedTypes;

    size_t _mappedBuildersSize;
    BuilderMapper **_mappedBuilders;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(TYPEREPLACER_INCL)
