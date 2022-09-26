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

#ifndef NATIVECALLABLECONTEXT_INCL
#define NATIVECALLABLECONTEXT_INCL

#include <string>
#include <vector>
#include "Context.hpp"
#include "BaseIterator.hpp"

namespace OMR {
namespace JitBuilder {

class Operation;
class Symbol;
class TextWriter;
class Type;
class TypeDictionary;

namespace Base {

class LocalSymbol;
class FunctionCompilation;
class ParameterSymbol;

class NativeCallableContext : public Context {
    friend class Function;

public:
    ParameterSymbol * DefineParameter(std::string name, const Type * type);
    LocalSymbol * DefineLocal(std::string name, const Type * type);
    void DefineReturnType(const Type * type) {
        _returnTypes.push_back(type);
    }

    LocalSymbolIterator LocalsBegin() const { return LocalSymbolIterator(this->_locals); }
    LocalSymbolIterator LocalsEnd() const { return endLocalSymbolIterator; }
    LocalSymbolVector ResetLocals() {
        LocalSymbolVector prev = this->_locals;
        this->_locals.clear();
        return prev;
    }

    ParameterSymbolIterator ParametersBegin() const { return ParameterSymbolIterator(this->_parameters); }
    ParameterSymbolIterator ParametersEnd() const { return endParameterSymbolIterator; }
    ParameterSymbolVector ResetParameters() {
        ParameterSymbolVector prev = this->_parameters;
        this->_parameters.clear();
        return prev;
    }

    int32_t numReturnTypes() const { return _returnTypes.size(); }
    const Type * returnType(int i=0) const {
        if (_returnTypes.size() > i)
            return this->_returnTypes[i];
        return NULL;
    }

protected:
    NativeCallableContext(FunctionCompilation *comp, std::string name="")
        : Context(comp, NULL, name) {

    }
    NativeCallableContext(FunctionCompilation *comp, NativeCallableContext *caller, std::string name="")
        : Context(comp, caller, name) {

    }

    void DefineParameter(ParameterSymbol *parm);
    void DefineLocal(LocalSymbol *local);

    ParameterSymbolVector _parameters;
    LocalSymbolVector _locals;
    std::vector<const Type *> _returnTypes;

    static LocalSymbolIterator endLocalSymbolIterator;
    static ParameterSymbolIterator endParameterSymbolIterator;
};

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // defined(NATIVECALLABLECONTEXT_INCL)

