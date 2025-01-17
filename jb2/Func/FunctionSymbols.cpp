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
#include "Func/FunctionSymbols.hpp"
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

INIT_JBALLOC_CAT(FunctionSymbol, (Symbol::allocCat()))

SUBCLASS_KINDSERVICE_IMPL(FunctionSymbol, "FunctionSymbol", Symbol, Extensible);

FunctionSymbol::~FunctionSymbol() {
}

FunctionSymbol::FunctionSymbol(Allocator *a, Extension *ext, IR *ir, const FunctionType *type, String name, String fileName, String lineNumber, void *entryPoint)
    : Symbol(a, getExtensibleClassKind(), ext, ir, name, type)
    , _fileName(fileName)
    , _lineNumber(lineNumber)
    , _entryPoint(entryPoint) {

}

FunctionSymbol::FunctionSymbol(Allocator *a, ExtensibleKind kind, Extension *ext, IR *ir, const FunctionType *type, String name, String fileName, String lineNumber, void *entryPoint)
    : Symbol(a, kind, ext, ir, name, type)
    , _fileName(fileName)
    , _lineNumber(lineNumber)
    , _entryPoint(entryPoint) {

}

FunctionSymbol::FunctionSymbol(Allocator *a, const FunctionSymbol *source, IRCloner *cloner)
    : Symbol(a, source, cloner)
    , _fileName(source->_fileName)
    , _lineNumber(source->_lineNumber)
    , _entryPoint(source->_entryPoint) {

}

Symbol *
FunctionSymbol::clone(Allocator *mem, IRCloner *cloner) const {
    assert(_kind == KIND(Extensible));
    return new (mem) FunctionSymbol(mem, this, cloner);
}

const FunctionType *
FunctionSymbol::functionType() const {
    return static_cast<const FunctionType *>(_type);
}

void
FunctionSymbol::logDetails(TextLogger & lgr) const {
   lgr << " Function";
}

INIT_JBALLOC_REUSECAT(LocalSymbol, Symbol)
SUBCLASS_KINDSERVICE_IMPL(LocalSymbol, "LocalSymbol", Symbol, Extensible);

LocalSymbol::LocalSymbol(Allocator *a, const LocalSymbol *source, IRCloner *cloner)
    : Symbol(a, source, cloner) {

}

Symbol *
LocalSymbol::clone(Allocator *mem, IRCloner *cloner) const {
    assert(_kind == KIND(Extensible));
    return new (mem) LocalSymbol(mem, this, cloner);
}

LocalSymbol::~LocalSymbol() {

}

void
LocalSymbol::logDetails(TextLogger & lgr) const {
   lgr << " Local";
}

INIT_JBALLOC_REUSECAT(ParameterSymbol, Symbol)
SUBCLASS_KINDSERVICE_IMPL(ParameterSymbol, "ParameterSymbol", LocalSymbol, Extensible);

ParameterSymbol::ParameterSymbol(Allocator *a, const ParameterSymbol *source, IRCloner *cloner)
    : LocalSymbol(a, source, cloner)
    , _index(source->_index) {

}

Symbol *
ParameterSymbol::clone(Allocator *mem, IRCloner *cloner) const {
    assert(_kind == KIND(Extensible));
    return new (mem) ParameterSymbol(mem, this, cloner);
}

ParameterSymbol::~ParameterSymbol() {

}

void
ParameterSymbol::logDetails(TextLogger & lgr) const {
   lgr << " Parameter";
}


} // namespace Func
} // namespace JB2
} // namespace OMR
