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

#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Extension.hpp"
#include "JB1MethodBuilder.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "TypeReplacer.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Type, TypeDictionary)

KindService Type::kindService;
TypeKind Type::TYPEKIND=KindService::NoKind;
bool Type::kindRegistered = false;

Type::Type(MEM_LOCATION(a), TypeKind kind, Extension *ext, String name, size_t size, const Type *layout)
    : Allocatable(a)
    , _ext(ext)
    , _createLoc(PASSLOC)
    , _dict(ext->compiler()->typeDict())
    , _id(_dict->getTypeID())
    , _kind(kind)
    , _name(name)
    , _size(size)
    , _layout(layout) {

    _dict->registerType(this);
}

Type::Type(MEM_LOCATION(a), TypeKind kind, Extension *ext, TypeDictionary *dict, String name, size_t size, const Type *layout)
    : Allocatable(a)
    , _ext(ext)
    , _createLoc(PASSLOC)
    , _dict(dict)
    , _id(dict->getTypeID())
    , _kind(kind)
    , _name(name)
    , _size(size)
    , _layout(layout) {

    dict->registerType(this);
}

Type::~Type() {

}

Literal *
Type::literal(LOCATION, Compilation *comp, const LiteralBytes *value) const {
    return comp->registerLiteral(PASSLOC, this, value);
}

String
Type::base_string(bool useHeader) const {
    String s;
    if (useHeader)
        s.append("type ");
    s.append("t").append(String::to_string(this->id())).append(" ");
    s.append(String::to_string(this->size())).append(" ");
    s.append(this->name()).append(" ");
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
    s.append(base_string()).append("primitiveType");
    if (_layout)
        s.append(" layout t").append(String::to_string(_layout->id())).append(" ").append(_layout->name());
    return s;
}

void
Type::transformTypeIfNeeded(TypeReplacer *repl, const Type *type) const {
    repl->transformTypeIfNeeded(type);
}

const TypeKind
Type::getTypeClassKind() {
    if (!kindRegistered) {
        // not really needed for Type class itself, but eatablishes general model for subclasses to use
        TYPEKIND = KindService::NoKind;
        kindRegistered = true;
    }
    return TYPEKIND;
}


//
// NoType
//

TypeKind NoTypeType::TYPEKIND = KindService::NoKind;
bool NoTypeType::kindRegistered = false;

NoTypeType::NoTypeType(MEM_LOCATION(a), Extension *ext)
    : Type(MEM_PASSLOC(a), TYPEKIND, ext, "NoType", 0) {

}

NoTypeType::~NoTypeType() {

}

const TypeKind
NoTypeType::getTypeClassKind() {
    if (!kindRegistered) {
        TYPEKIND = KindService::NoKind;
        kindRegistered = true;
    }
    return TYPEKIND;
}

void
NoTypeType::logValue(TextLogger &lgr, const void *p) const {
    lgr << name();
}

bool
NoTypeType::registerJB1Type(JB1MethodBuilder *j1mb) const {
    j1mb->registerNoType(this);
    return true;
}

} // namespace JitBuilder
} // namespace OMR
