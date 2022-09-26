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

#ifndef FUNCTION_INCL
#define FUNCTION_INCL

#include <exception>
#include <string>
#include <vector>

#include "BaseExtension.hpp"
#include "BaseIterator.hpp"
#include "BaseSymbols.hpp"
#include "BaseTypes.hpp"
#include "Builder.hpp"
#include "Config.hpp"
#include "IDs.hpp"
#include "Iterator.hpp"
#include "typedefs.hpp"

#define TOSTR(x) #x
#define LINETOSTR(x) TOSTR(x)

namespace OMR {
namespace JitBuilder {

class Operation;
class Symbol;
class TextWriter;
class TypeDictionary;

namespace Base {

class Debugger;
class FunctionCompilation;
class NativeCallableContext;

class Function {
    friend class FunctionCompilation;

public:
    virtual ~Function();

    void DefineName(std::string name);
    void DefineFile(std::string file);
    void DefineLine(std::string line);
    ParameterSymbol * DefineParameter(std::string name, const Type * type);
    void DefineReturnType(const Type * type);
    LocalSymbol * DefineLocal(std::string name, const Type * type);
    FunctionSymbol * DefineFunction(LOCATION, std::string name, std::string fileName, std::string lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, ...);
    FunctionSymbol * DefineFunction(LOCATION, std::string name, std::string fileName, std::string lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);
    const PointerType * PointerTo(LOCATION, const Type *baseType);

    std::string name() const { return _givenName; }
    std::string fileName() const { return _fileName; }
    std::string lineNumber() const { return _lineNumber; }

    ParameterSymbolIterator ParametersBegin() const;
    ParameterSymbolIterator ParametersEnd() const;
    ParameterSymbolVector ResetParameters();

    LocalSymbolIterator LocalsBegin() const;
    LocalSymbolIterator LocalsEnd() const;
    LocalSymbolVector ResetLocals();
    LocalSymbol * LookupLocal(std::string name);

    FunctionSymbolIterator FunctionsBegin() const { return FunctionSymbolIterator(_functions); }
    FunctionSymbolIterator FunctionsEnd() const { return endFunctionIterator; }
    FunctionSymbolVector ResetFunctions();
    FunctionSymbol *LookupFunction(std::string name) {
        Symbol *sym = getSymbol(name);
        if (sym == NULL || !sym->isKind<FunctionSymbol>())
            return NULL;
        return sym->refine<FunctionSymbol>();
    }

    int32_t numEntryPoints() const { return _numEntryPoints; }
    Builder *builderEntry(int32_t i=0) const { return this->_entryPoints[i]; }

    int32_t numValues() const;
    int32_t numLocals() const;
    int32_t numReturnValues() const;

    const Type * returnType() const;
    TypeDictionary * dict() const { return _dict; }
    FunctionCompilation *comp() const { return _comp; }
    Config * config() const;

    bool constructIL() {
        bool rc = buildIL();
        return rc;
    }

    virtual bool buildIL() {
        return true;
    }

    CompilerReturnCode Compile(TextWriter *logger=NULL, StrategyID strategy=NoStrategy);

    template<typename T>
    T nativeEntry(int i=0) const {
        assert(i < _numEntryPoints);
        assert(_nativeEntryPoints[i]);
        return reinterpret_cast<T>(_nativeEntryPoints[i]);
    }

    #if 0
    template<typename T>
    T * debugEntry() const { return reinterpret_cast<T *>(_debugEntryPoint); }
    #endif

    void write(TextWriter & w) const;

    void constructJB1Function(JB1MethodBuilder *j1mb);
    void jbgenProlog(JB1MethodBuilder *j1mb);
    void setNativeEntryPoint(void *entry, int i=0) {
        if (i < _numEntryPoints)
            _nativeEntryPoints[i] = entry;
    }

#if 0
    template<typename T>
    T * NativeEntry(int i=0) const { return reinterpret_cast<T *>(_nativeEntryPoint[i]); }

    template<typename T> T *entryPoint() const { assert(_entryPoint != NULL); return reinterpret_cast<T *>(_entryPoint); }

    template<typename T> T *DebugEntry(int32_t *returnCode);
    template<typename T> T *DebugEntry(int32_t *returnCode) const { return reinterpret_cast<T *>(internalDebugger(returnCode)); }
    template<typename T> T *debugEntryPoint() const { assert(_debugEntryPoint != NULL); return reinterpret_cast<T *>(_debugEntryPoint); }
#endif

    Symbol * getSymbol(std::string name);
    void addLocation(Location *loc ) { _locations.push_back(loc); }

    virtual void replaceTypes(TypeReplacer *repl);

protected:
    Function(Compiler *compiler); // meant to be subclassed
    Function(Function *outerFunction);

    void DefineParameter(ParameterSymbol *parm);
    void DefineLocal(LocalSymbol *local);
    void DefineFunction(FunctionSymbol *function);
    FunctionSymbol * internalDefineFunction(LOCATION, std::string name, std::string fileName, std::string lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);
    void addInitialBuildersToWorklist(BuilderWorklist & worklist);

    #if 0
    void *internalCompile(int32_t *returnCode);
    void *internalDebugger(int32_t *returnCode);
    #endif

    Compiler              * _compiler;
    BaseExtension         * _ext;
    Function              * _outerFunction;
    TypeDictionary        * _dict;
    FunctionCompilation   * _comp;
    NativeCallableContext * _nativeContext;

    std::string             _givenName;
    std::string             _fileName;
    std::string             _lineNumber;
    FunctionSymbolVector    _functions; // move to compiler?

    std::vector<Location *> _locations;

    int32_t                 _numEntryPoints;
    Builder              ** _entryPoints;
    void                 ** _nativeEntryPoints;
    void                 ** _debugEntryPoints;
    Debugger              * _debuggerObject;

    static FunctionSymbolIterator endFunctionIterator;
};

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // defined(FUNCTION_INCL)
