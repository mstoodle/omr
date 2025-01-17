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
namespace JB2 {

INIT_JBALLOC_ON(Type, TypeDictionary)
SUBCLASS_KINDSERVICE_IMPL(Type,"Type",ExtensibleIR, Extensible)

// For installation into the Compiler's prototype IR
Type::Type(MEM_LOCATION(a), ExtensibleKind kind, Extension *ext, String name, size_t size, const Type *layout)
    : ExtensibleIR(a, ext, ext->compiler(), kind)
    , _ext(ext)
    , _createLoc(PASSLOC)
    , _id(ext->compiler()->irPrototype()->getTypeID())
    , _name(name)
    , _size(size)
    , _layout(layout) {

    ir()->typedict()->registerType(this);
}

Type::Type(MEM_LOCATION(a), ExtensibleKind kind, Extension *ext, IR *ir, String name, size_t size, const Type *layout)
    : ExtensibleIR(a, ext, ir, kind)
    , _ext(ext)
    , _createLoc(PASSLOC)
    , _id(ir->getTypeID())
    , _name(name)
    , _size(size)
    , _layout(layout) {

    ir->typedict()->registerType(this);
}

Type::Type(MEM_LOCATION(a), ExtensibleKind kind, Extension *ext, IR *ir, TypeID tid, String name, size_t size, const Type *layout)
    : ExtensibleIR(a, ext, ir, kind)
    , _ext(ext)
    , _createLoc(PASSLOC)
    , _id(tid)
    , _name(name)
    , _size(size)
    , _layout(layout) {

    ir->typedict()->registerType(this);
}

// only used by clone
Type::Type(Allocator *a, const Type *source, IRCloner *cloner)
    : ExtensibleIR(a, source, cloner)
    , _ext(source->_ext)
    , _createLoc(source->_createLoc)
    , _id(source->_id)
    , _name(source->_name)
    , _size(source->_size)
    , _layout(NULL) {
    
    if (source->_layout)
        _layout = cloner->clonedType(source->_layout);
}

Type::~Type() {

}

const Type *
Type::cloneType(Allocator *a, IRCloner *cloner) const {
    assert(0); // Should not be any Type objets
    assert(_kind == KIND(Extensible));
    return new (a) Type(a, this, cloner);
}

Literal *
Type::literal(LOCATION, const LiteralBytes *value) const {
    return ir()->registerLiteral(PASSLOC, this, value);
}

String
Type::base_string(Allocator *mem, bool useHeader) const {
    String s(mem);
    if (useHeader)
        s.append("type ");
    s.append(String(mem, "t"));
    s.append(String::to_string(mem, this->id()));
    s.append(String(mem, " "));
    s.append(String::to_string(mem, this->size()));
    s.append(String(mem, " "));
    s.append(this->name());
    s.append(String(mem, " "));
    return s;
}

void
Type::log(TextLogger &lgr, bool indent) const {
    lgr.irOneLinerBegin("type", "t", _id);
    this->logContents(lgr);
    lgr.irOneLinerEnd();
}

void
Type::logContents(TextLogger & lgr) const {
     lgr << "size " << size() << " " << name() << " primitiveType";
}

void
Type::logType(TextLogger &lgr, bool useHeader) const {
    lgr << "[ ";
    lgr << this->to_string(_ext->compiler()->mem(), useHeader);
    lgr << " ]";
}

String
Type::to_string(Allocator *mem, bool useHeader) const {
    String s(mem);
    s.append(base_string(mem, useHeader));
    s.append("primitiveType");
    if (_layout)
        s.append(" layout t").append(String::to_string(mem, _layout->id())).append(String(mem, " ")).append(_layout->name());
    return s;
}

void
Type::transformTypeIfNeeded(TypeReplacer *repl, const Type *type) const {
    repl->transformTypeIfNeeded(type);
}


//
// NoType
//

DEFINE_TYPE_CLASS(NoTypeType, Type, "NoType",
    void
    NoTypeType::logValue(TextLogger &lgr, const void *p) const {
        lgr << "NoType";
    }

    void
    NoTypeType::logLiteral(TextLogger & lgr, const Literal *lv) const {
        assert(0);
    }

    bool
    NoTypeType::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
        return false;
    }
)


} // namespace JB2
} // namespace OMR
