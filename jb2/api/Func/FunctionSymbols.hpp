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

#ifndef FUNCTIONSYMBOLS_INCL
#define FUNCTIONSYMBOLS_INCL

#include <string>
#include "Func/FunctionSymbols.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

class FunctionType;

class LocalSymbol : public Symbol {
    friend class FunctionExtension;

public:
    LocalSymbol(std::string name, const Type * type)
        : Symbol(getSymbolClassKind(), name, type) {
    }

    static const SymbolKind getSymbolClassKind();

protected:
    LocalSymbol(SymbolKind kind, std::string name, const Type * type)
        : Symbol(kind, name, type) {
    }

    static SymbolKind SYMBOLKIND;
    static bool kindRegistered;
};

class FunctionSymbol : public Symbol {
    friend class FunctionExtension;
    friend class Function;

public:
    FunctionSymbol(const FunctionType *type, std::string name, std::string fileName, std::string lineNumber, void *entryPoint);
    const FunctionType *functionType() const;
    std::string fileName() const { return _fileName; }
    std::string lineNumber() const { return _lineNumber; }
    void *entryPoint() const { return _entryPoint; }

    static const SymbolKind getSymbolClassKind();

protected:
    std::string _fileName;
    std::string _lineNumber;
    void *_entryPoint;

    static SymbolKind SYMBOLKIND;
    static bool kindRegistered;
};

class ParameterSymbol : public LocalSymbol {
    friend class FunctionExtension;

public:
    ParameterSymbol(std::string name, const Type * type, int index)
        : LocalSymbol(getSymbolClassKind(), name, type)
        , _index(index) {

    }

    int index() const { return _index; }

    static const SymbolKind getSymbolClassKind();

protected:
    int _index;

    static SymbolKind SYMBOLKIND;
    static bool kindRegistered;
};

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(FUNCTIONSYMBOLS_INCL)