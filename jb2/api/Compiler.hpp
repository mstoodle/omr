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

#ifndef COMPILER_INCL
#define COMPILER_INCL

#include "common.hpp"
#include <map>
#include "AllocationCategoryService.hpp"
#include "AllocatorRaw.hpp"
#include "Extensible.hpp"
#include "CreateLoc.hpp"
#include "KindService.hpp"
#include "String.hpp"

namespace OMR {
namespace JitBuilder {

class Compilation;
class CompilationException;
class CompiledBody;
class CompileUnit;
class Config;
class CoreExtension;
class Extension;
class LiteralDictionary;
class Pass;
class Platform;
class SemanticVersion;
class Strategy;
class TextWriter;
class Type;
class SymbolDictionary;
class TypeDictionary;


class Compiler : public Allocatable {
    JBALLOC_(Compiler)

    friend class Allocator;
    friend class Compilation;
    friend class CompilationException;
    friend class CompiledBody;
    friend class CompileUnit;
    friend class Context;
    friend class Executor;
    friend class Extensible;
    friend class Extension;
    friend class LiteralDictionary;
    friend class Pass;
    friend class Strategy;
    friend class SymbolDictionary;
    friend class TypeDictionary;

public:
    ALL_ALLOC_ALLOWED(Compiler, String name, Config *config=NULL);
    ALL_ALLOC_ALLOWED(Compiler, Compiler *parent, String name, Config *config=NULL);

    CompilerID id() const { return _id; }
    String name() const { return _name; }
    Config *config() const { return _config; }
    Compiler *parent() const { return _parent; }
    Allocator *mem() const { return _mem; }
    LiteralDictionary *litDict() const { return _litDict; }
    SymbolDictionary *symDict() const { return _symDict; }
    TypeDictionary *typeDict() const { return _typeDict; }
    TextWriter *textWriter(TextLogger & lgr);

    ExtensionID getExtensionID() { return _nextExtensionID++; }
    CoreExtension *coreExt() const { return _core; }
    template<typename T>
    T *loadExtension(LOCATION, const SemanticVersion *version=NULL, String name=T::NAME) {
        return static_cast<T *>(internalLoadExtension(PASSLOC, version, name));
    }
    template<typename T>
    T *lookupExtension(String name=T::NAME) {
        return static_cast<T *>(internalLookupExtension(name));
    }
    bool validateExtension(String name) const;

    void registerExtensible(Extensible *e, KINDTYPE(Extensible) kind);

    PassID lookupPass(String name);
    Strategy * lookupStrategy(StrategyID id);
    CompilerReturnCode compile(LOCATION, Compilation *comp, StrategyID strategyID);

    List<Extensible *>::Iterator extensibles(KINDTYPE(Extensible) kind) const {
        auto it=_extensiblesByKind.find(kind);
        if (it == _extensiblesByKind.end())
            return List<Extensible *>::Iterator(); // empty iterator
        List<Extensible *> *list = it->second;
        return list->iterator();
    }

    const String actionName(ActionID a) const {
        assert(a < _nextActionID);
        auto found = _actionNames.find(a);
        assert(found != _actionNames.end());
        return found->second;
    }

    const String returnCodeName(CompilerReturnCode c) const {
        assert(c < _nextReturnCode);
        auto found = _returnCodeNames.find(c);
        assert(found != _returnCodeNames.end());
        return found->second;
    }

    uint8_t platformWordSize() const { return 64; } // should test _targetPlatform!

    bool hasErrorCondition() const { return _errorCondition != NULL; }
    CompilationException *errorCondition() const { return _errorCondition; } 
    void clearErrorCondition();

protected:
    void init();

    void addExtension(Extension *ext);
    void createAnyAddons(Extensible *e, KINDTYPE(Extensible) kind);
    void registerForExtensible(ExtensibleKind kind, Extension *ext);

    PassID addPass(Pass *pass);
    StrategyID addStrategy(Strategy *st);

    LiteralDictionaryID maxLiteralDictionaryID() const { return _nextLiteralDictionaryID-1; }
    SymbolDictionaryID maxSymbolDictionaryID() const { return _nextSymbolDictionaryID-1; }
    SymbolDictionaryID maxTypeDictionaryID() const { return _nextTypeDictionaryID-1; }

    LiteralDictionaryID getLiteralDictionaryID() { return _nextLiteralDictionaryID++; }
    SymbolDictionaryID getSymbolDictionaryID() { return _nextSymbolDictionaryID++; }
    TypeDictionaryID getTypeDictionaryID() { return this->_nextTypeDictionaryID++; }

    Extension *internalLoadExtension(LOCATION, const SemanticVersion * version, String name);
    Extension *internalLookupExtension(String name);

    void extensionCouldNotLoad(LOCATION, String name, char *dlerrorMsg);
    void extensionHasNoCreateFunction(LOCATION, Extension *ext, String name, char *dlerrorMsg);
    void extensionCouldNotCreate(LOCATION, String name);
    void extensionVersionMismatch(LOCATION, String name, const SemanticVersion *version, const SemanticVersion *extensionVersion);

    void notifyRecompile(CompileUnit *unit, CompiledBody *oldBody, CompiledBody *newBody, StrategyID strategy=NoStrategy);

    uint64_t _eyeCatcher;
    CompilerID _id;
    String _name;
    AllocatorRaw _mallocAllocator;
    Allocator *_baseAllocator; // must come after _mallocAllocator
    bool _myConfig;
    Config *_config; // must come after _baseAllocator and before _mem
    Allocator *_mem; // must come after _config;
    Compiler *_parent;

    ExtensionID _nextExtensionID;
    std::map<String, Extension *> _extensions;
    CoreExtension *_core;

    std::map<KINDTYPE(Extensible), List<Extensible *> *> _extensiblesByKind;
    std::map<KINDTYPE(Extensible), List<Extension *> *> _extensionsForAddonsByKind;

    ActionID assignActionID(String name);
    ActionID _nextActionID;
    std::map<ActionID, String> _actionNames;

    PassID _nextPassID;
    std::map<String, PassID> _passNames;

    CompilationID getCompilationID() { return this->_nextCompilationID++; }
    CompilationID _nextCompilationID;

    CompileUnitID getCompiledBodyID() { return this->_nextCompiledBodyID++; }
    CompileUnitID _nextCompiledBodyID;

    CompileUnitID getCompileUnitID() { return this->_nextCompileUnitID++; }
    CompileUnitID _nextCompileUnitID;

    ContextID getContextID() { return this->_nextContextID++; }
    ContextID _nextContextID;

    ExecutorID getExecutorID() { return this->_nextExecutorID++; }
    ExecutorID _nextExecutorID;

    CompilerReturnCode assignReturnCode(String name);
    CompilerReturnCode _nextReturnCode;
    std::map<CompilerReturnCode, String> _returnCodeNames;

    StrategyID _nextStrategyID;
    std::map<StrategyID, Strategy *> _strategies;

    TypeID _nextTypeID;
    std::map<TypeID, Type *> _types;

    LiteralDictionaryID _nextLiteralDictionaryID;
    SymbolDictionaryID _nextSymbolDictionaryID;
    TypeDictionaryID _nextTypeDictionaryID;

    Platform *_target;
    Platform *_compiler;

    // must come AFTER _nextTypeDictionaryID for proper initialization
    LiteralDictionary *_litDict;
    SymbolDictionary *_symDict;
    TypeDictionary *_typeDict;
    
    List<TextWriter *> _textWriters;

    CompilationException *_errorCondition;

    static EyeCatcher EYE_CATCHER_COMPILER;
    static CompilerID nextCompilerID;

// put these at end so they're initialized after everthing else
public:
    CompilerReturnCode CompileSuccessful;
    CompilerReturnCode CompileNotStarted;
    CompilerReturnCode CompileFailed;
    CompilerReturnCode CompileFail_CompilerError;
    CompilerReturnCode CompileFail_UnknownStrategyID;
    CompilerReturnCode CompileFail_IlGen;
    CompilerReturnCode CompileFail_TypeMustBeReduced;
    CompilerReturnCode CompilerError_Extension_CouldNotLoad;
    CompilerReturnCode CompilerError_Extension_HasNoCreateFunction;
    CompilerReturnCode CompilerError_Extension_CouldNotCreate;
    CompilerReturnCode CompilerError_Extension_VersionMismatch;

    StrategyID jb1cgStrategyID;
};

class CompilationException : public Allocatable, public std::exception {
    JBALLOC(CompilationException, NoAllocationCategory)

public:
    ALL_ALLOC_ALLOWED(CompilationException, LOCATION, Compiler *compiler, CompilerReturnCode result);

    CompilerReturnCode result() const { return _result; }
    String resultString() const { return _compiler->returnCodeName(_result); }

    String location() const     { return            String(_location.to_string()) ; }
    String locationLine() const { return String(addNewLine(_location.to_string())); }

    String message() const { return _message; }

    CompilationException & setMessage(String str)     { _message =            str ; return *this; }
    CompilationException & setMessageLine(String str) { _message = addNewLine(str); return *this; }

    CompilationException & appendMessage(String str)     { _message.append(           str ); return *this; }
    CompilationException & appendMessageLine(String str) { _message.append(addNewLine(str)); return *this; }

protected:
    const Compiler *_compiler;
    CompilerReturnCode _result;
    CreateLocation _location;
    String _message;

    String addNewLine(String s) const { return s + String("\n"); }
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(COMPILER_INCL)
