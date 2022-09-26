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

#include "BaseSymbols.hpp"
#include "BaseTypes.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

// should be initialized first time it's needed by ensureKindRegistered()
bool LocalSymbol::kindRegistered=false;
SymbolKind LocalSymbol::SYMBOLKIND=KindService::NoKind;

const SymbolKind
LocalSymbol::getSymbolClassKind() {
    if (!kindRegistered) {
        SYMBOLKIND = Symbol::kindService.assignKind(KindService::AnyKind, "LocalSymbol");
	kindRegistered = true;
    }
    return SYMBOLKIND;
}

// should be initialized first time it's needed by ensureKindRegistered()
bool FieldSymbol::kindRegistered=false;
SymbolKind FieldSymbol::SYMBOLKIND=KindService::NoKind;

const SymbolKind
FieldSymbol::getSymbolClassKind() {
    if (!kindRegistered) {
        SYMBOLKIND = Symbol::kindService.assignKind(KindService::AnyKind, "FieldSymbol");
	kindRegistered = true;
    }
    return SYMBOLKIND;
}

// should be initialized first time it's needed by ensureKindRegistered()
bool FunctionSymbol::kindRegistered=false;
SymbolKind FunctionSymbol::SYMBOLKIND=KindService::NoKind;

const SymbolKind
FunctionSymbol::getSymbolClassKind() {
    if (!kindRegistered) {
        SYMBOLKIND = Symbol::kindService.assignKind(KindService::AnyKind, "FunctionSymbol");
	kindRegistered = true;
    }
    return SYMBOLKIND;
}

// should be initialized first time it's needed by ensureKindRegistered()
bool ParameterSymbol::kindRegistered=false;
SymbolKind ParameterSymbol::SYMBOLKIND=KindService::NoKind;

const SymbolKind
ParameterSymbol::getSymbolClassKind() {
    if (!kindRegistered) {
        SYMBOLKIND = Symbol::kindService.assignKind(LocalSymbol::getSymbolClassKind(), "ParameterSymbol");
	kindRegistered = true;
    }
    return SYMBOLKIND;
}


FunctionSymbol::FunctionSymbol(const FunctionType *type, std::string name, std::string fileName, std::string lineNumber, void *entryPoint)
    : Symbol(getSymbolClassKind(), name, type)
    , _fileName(fileName)
    , _lineNumber(lineNumber)
    , _entryPoint(entryPoint) {

}

const FunctionType *
FunctionSymbol::functionType() const {
    return static_cast<const FunctionType *>(_type);
}

FieldSymbol::FieldSymbol(std::string name, const StructType *structType, const FieldType *fieldType)
    : Symbol(getSymbolClassKind(), name, fieldType->type())
    , _structType(structType)
    , _fieldType(fieldType) {

}

} // namespace Base
} // namespace JitBuilder
} // namespace OMR
