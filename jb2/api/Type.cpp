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

#include "Compiler.hpp"
#include "Extension.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "TypeReplacer.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Type, TypeDictionary)
BASECLASS_KINDSERVICE_IMPL(Type)

Type::Type(MEM_LOCATION(a), TypeKind kind, Extension *ext, String name, size_t size, const Type *layout)
    : Allocatable(a)
    , _ext(ext)
    , _createLoc(PASSLOC)
    , _dict(ext->compiler()->typedict())
    , _id(ext->compiler()->typedict()->getTypeID())
    , _name(name)
    , _size(size)
    , _layout(layout)
    , BASECLASS_KINDINIT(kind) {

    ext->compiler()->typedict()->registerType(this);
}

Type::Type(MEM_LOCATION(a), TypeKind kind, Extension *ext, TypeDictionary *dict, String name, size_t size, const Type *layout)
    : Allocatable(a)
    , _ext(ext)
    , _createLoc(PASSLOC)
    , _dict(dict)
    , _id(dict->getTypeID())
    , _name(name)
    , _size(size)
    , _layout(layout)
    , BASECLASS_KINDINIT(kind) {

    dict->registerType(this);
}

// only used by clone
Type::Type(Allocator *a, const Type *source, IRCloner *cloner)
    : Allocatable(a)
    , _ext(source->_ext)
    , _createLoc(source->_createLoc)
    , _dict(cloner->clonedTypeDictionary(source->_dict))
    , _id(source->_id)
    , _name(source->_name)
    , _size(source->_size)
    , _layout(NULL)
    , BASECLASS_KINDINIT(KIND(Type)) {
    
    if (source->_layout)
        _layout = cloner->clonedType(source->_layout);
    _dict->registerType(this);
}

Type::~Type() {

}

const Type *
Type::clone(Allocator *a, IRCloner *cloner) const {
    assert(0); // Should not be any Type objets
    assert(_kind == KIND(Type));
    return new (a) Type(a, this, cloner);
}

const Type *
Type::cloneType(Allocator *a, IRCloner *cloner) const {
    return clone(a, cloner);
}

Literal *
Type::literal(LOCATION, IR *ir, const LiteralBytes *value) const {
    return ir->registerLiteral(PASSLOC, this, value);
}

String
Type::base_string(bool useHeader) const {
    String s;
    if (useHeader)
        s.append("type ");
    s.append(String("t"));
    s.append(String::to_string(this->id()));
    s.append(String(" "));
    s.append(String::to_string(this->size()));
    s.append(String(" "));
    s.append(this->name());
    s.append(String(" "));
    return s;
}

void
Type::logType(TextLogger &lgr, bool useHeader) const {
    lgr << "[ ";
    lgr << this->to_string(useHeader);
    lgr << " ]";
}

String
Type::to_string(bool useHeader) const {
    String s;
    s.append(base_string(useHeader));
    s.append("primitiveType");
    if (_layout)
        s.append(" layout t").append(String::to_string(_layout->id())).append(String(" ")).append(_layout->name());
    return s;
}

void
Type::transformTypeIfNeeded(TypeReplacer *repl, const Type *type) const {
    repl->transformTypeIfNeeded(type);
}


//
// NoType
//

SUBCLASS_KINDSERVICE_IMPL(NoTypeType, "NoType", Type, Type);

NoTypeType::NoTypeType(MEM_LOCATION(a), Extension *ext)
    : Type(MEM_PASSLOC(a), getTypeClassKind(), ext, "NoType", 0) {

}

NoTypeType::NoTypeType(Allocator *a, const Type *source, IRCloner *cloner)
    : Type(a, source, cloner) {

}

NoTypeType::~NoTypeType() {

}

const Type *
NoTypeType::clone(Allocator *a, IRCloner *cloner) const {
    assert(_kind == KIND(Type));
    return new (a) NoTypeType(a, this, cloner);
}

void
NoTypeType::logValue(TextLogger &lgr, const void *p) const {
    lgr << name();
}


} // namespace JitBuilder
} // namespace OMR
