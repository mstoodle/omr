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

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class FunctionType;

class FunctionSymbol : public Symbol {
    JBALLOC_(FunctionSymbol);

    friend class FunctionExtension;
    friend class Function;
public:
    FunctionSymbol(Allocator *a, Extension *ext, IR *ir, const FunctionType *type, String name, String fileName, String lineNumber, void *entryPoint);
    FunctionSymbol(Allocator *a, ExtensibleKind kind, Extension *ext, IR *ir, const FunctionType *type, String name, String fileName, String lineNumber, void *entryPoint);
    const FunctionType *functionType() const;
    String fileName() const { return _fileName; }
    String lineNumber() const { return _lineNumber; }
    void *entryPoint() const { return _entryPoint; }
    virtual void logDetails(TextLogger & lgr) const;

protected:
    FunctionSymbol(Allocator *a, const FunctionSymbol *source, IRCloner *cloner);
    virtual Symbol *clone(Allocator *a, IRCloner *cloner) const;

    String _fileName;
    String _lineNumber;
    void *_entryPoint;

    SUBCLASS_KINDSERVICE_DECL(Extensible, FunctionSymbol);
};

class LocalSymbol : public Symbol {
    JBALLOC_(LocalSymbol)

    friend class FunctionExtension;
public:
    LocalSymbol(Allocator *a, Extension *ext, IR *ir, String name, const Type * type)
        : Symbol(a, getExtensibleClassKind(), ext, ir, name, type) {
    }
    virtual void logDetails(TextLogger & lgr) const;

protected:
    LocalSymbol(Allocator *a, ExtensibleKind kind, Extension *ext, IR *ir, String name, const Type * type)
        : Symbol(a, kind, ext, ir, name, type) {
    }
    LocalSymbol(Allocator *a, const LocalSymbol *source, IRCloner *cloner);
    virtual Symbol *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Extensible, LocalSymbol);
};

class ParameterSymbol : public LocalSymbol {
    JBALLOC_(ParameterSymbol)

    friend class FunctionExtension;
public:
    ParameterSymbol(Allocator *a, Extension *ext, IR *ir, String name, const Type * type, int index)
        : LocalSymbol(a, getExtensibleClassKind(), ext, ir, name, type)
        , _index(index) {

    }

    int index() const { return _index; }
    virtual void logDetails(TextLogger & lgr) const;

protected:
    ParameterSymbol(Allocator *a, const ParameterSymbol *source, IRCloner *cloner);
    virtual Symbol *clone(Allocator *a, IRCloner *cloner) const;

    int _index;

    SUBCLASS_KINDSERVICE_DECL(Extensible, ParameterSymbol);
};

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // !defined(FUNCTIONSYMBOLS_INCL)
