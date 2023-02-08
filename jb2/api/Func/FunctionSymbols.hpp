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

#include "Func/FunctionSymbols.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

class FunctionType;

class FunctionSymbol : public Symbol {
    JBALLOC_(FunctionSymbol);

    friend class FunctionExtension;
    friend class Function;
public:
    FunctionSymbol(Allocator *a, const FunctionType *type, String name, String fileName, String lineNumber, void *entryPoint);
    const FunctionType *functionType() const;
    String fileName() const { return _fileName; }
    String lineNumber() const { return _lineNumber; }
    void *entryPoint() const { return _entryPoint; }

    static const SymbolKind getSymbolClassKind();

protected:
    String _fileName;
    String _lineNumber;
    void *_entryPoint;

    static SymbolKind SYMBOLKIND;
    static bool kindRegistered;
};

class LocalSymbol : public Symbol {
    JBALLOC_(LocalSymbol)

    friend class FunctionExtension;
public:
    LocalSymbol(Allocator *a, String name, const Type * type)
        : Symbol(a, getSymbolClassKind(), name, type) {
    }

    static const SymbolKind getSymbolClassKind();

protected:
    LocalSymbol(Allocator *a, SymbolKind kind, String name, const Type * type)
        : Symbol(a, kind, name, type) {
    }

    static SymbolKind SYMBOLKIND;
    static bool kindRegistered;
};

class ParameterSymbol : public LocalSymbol {
    JBALLOC_(ParameterSymbol)

    friend class FunctionExtension;
public:
    ParameterSymbol(Allocator *a, String name, const Type * type, int index)
        : LocalSymbol(a, getSymbolClassKind(), name, type)
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
