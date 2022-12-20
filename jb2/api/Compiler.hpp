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
#include "CreateLoc.hpp"
#include "util/String.hpp"

namespace OMR {
namespace JitBuilder {

class Compilation;
class CompilationException;
class CompiledBody;
class CompileUnit;
class Config;
class Extension;
class JB1;
class Pass;
class Platform;
class SemanticVersion;
class Strategy;
class Type;
class TypeDictionary;

class Compiler {
    friend class Compilation;
    friend class CompilationException;
    friend class CompiledBody;
    friend class CompileUnit;
    friend class Context;
    friend class Extension;
    friend class Pass;
    friend class Strategy;
    friend class TypeDictionary;

    typedef Array<Pass *> PassChain;
    typedef std::map<PassID, PassChain> PassRegistry;

public:
    Compiler(String name, Config *config=NULL, Compiler *parent=NULL);
    virtual ~Compiler();

    CompilerID id() const { return _id; }
    String name() const { return _name; }
    Config *config() const { return _config; }
    Compiler *parent() const { return _parent; }
    TypeDictionary *dict() const { return _dict; }

    ExtensionID getExtensionID() { return _nextExtensionID++; }
    template<typename T>
    T *loadExtension(LOCATION, const SemanticVersion *version=NULL, String name=T::NAME) {
        return static_cast<T *>(internalLoadExtension(PASSLOC, version, name));
    }
    template<typename T>
    T *lookupExtension(String name=T::NAME) {
        return static_cast<T *>(internalLookupExtension(name));
    }
    bool validateExtension(String name) const;

    PassID lookupPass(String name);
    CompilerReturnCode compile(LOCATION, Compilation *comp, StrategyID strategyID);

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

protected:
    void addExtension(Extension *ext);
    PassID addPass(Pass *pass);
    StrategyID addStrategy(Strategy *st);
    TypeDictionaryID getTypeDictionaryID() {
        return this->_nextTypeDictionaryID++;
    }

    Extension *internalLoadExtension(LOCATION, const SemanticVersion * version, String name);
    Extension *internalLookupExtension(String name);
    Strategy * lookupStrategy(StrategyID id);

    void extensionCouldNotLoad(LOCATION, String name, char *dlerrorMsg);
    void extensionHasNoCreateFunction(LOCATION, Extension *ext, String name, char *dlerrorMsg);
    void extensionCouldNotCreate(LOCATION, String name);
    void extensionVersionMismatch(LOCATION, String name, const SemanticVersion *version, const SemanticVersion *extensionVersion);

    void notifyRecompile(CompileUnit *unit, CompiledBody *oldBody, CompiledBody *newBody, StrategyID strategy=NoStrategy);

    uint64_t _eyeCatcher;
    CompilerID _id;
    String _name;
    bool _myConfig;
    Config *_config;
    Compiler *_parent;

    JB1 *_jb1;

    ExtensionID _nextExtensionID;
    std::map<String, Extension *> _extensions;

    ActionID assignActionID(String name);
    ActionID _nextActionID;
    std::map<ActionID, String> _actionNames;

    PassID _nextPassID;
    std::map<String, PassID> _registeredPassNames;
    PassRegistry _passRegistry;

    CompilationID getCompilationID() { return this->_nextCompilationID++; }
    CompilationID _nextCompilationID;

    CompileUnitID getCompiledBodyID() { return this->_nextCompiledBodyID++; }
    CompileUnitID _nextCompiledBodyID;

    CompileUnitID getCompileUnitID() { return this->_nextCompileUnitID++; }
    CompileUnitID _nextCompileUnitID;

    ContextID getContextID() { return _nextContextID++; }
    ContextID _nextContextID;

    CompilerReturnCode assignReturnCode(String name);
    CompilerReturnCode _nextReturnCode;
    std::map<CompilerReturnCode, String> _returnCodeNames;

    StrategyID _nextStrategyID;
    std::map<StrategyID, Strategy *> _strategies;

    TypeID _nextTypeID;
    std::map<TypeID, Type *> _types;

    TypeDictionaryID _nextTypeDictionaryID;

    Platform *_target;
    Platform *_compiler;

    // must come AFTER _nextTypeDictionaryID for proper initialization
    TypeDictionary *_dict;

    CompilationException *_errorCondition;

    static EyeCatcher EYE_CATCHER_COMPILER;
    static CompilerID nextCompilerID;

// put these at end so they're initialized after everthing else
public:
    CompilerReturnCode CompileSuccessful;
    CompilerReturnCode CompileNotStarted;
    CompilerReturnCode CompileFailed;
    CompilerReturnCode CompileFail_UnknownStrategyID;
    CompilerReturnCode CompileFail_IlGen;
    CompilerReturnCode CompileFail_TypeMustBeReduced;
    CompilerReturnCode CompilerError_Extension_CouldNotLoad;
    CompilerReturnCode CompilerError_Extension_HasNoCreateFunction;
    CompilerReturnCode CompilerError_Extension_CouldNotCreate;
    CompilerReturnCode CompilerError_Extension_VersionMismatch;

    StrategyID jb1cgStrategyID;
};

class CompilationException : public std::exception {
public:
    CompilationException(LOCATION, Compiler *compiler, CompilerReturnCode result)
        : std::exception()
        , _compiler(compiler)
        , _result(result)
        , _location(PASSLOC)
        , _message(String("CompilationException")) {
    }

    CompilerReturnCode result() const { return _result; }
    String resultString() const { return _compiler->returnCodeName(_result); }

    String location() const     { return            String(_location.to_string()) ; }
    String locationLine() const { return String(addNewLine(_location.to_string())); }

    String message() const { return _message; }

    CompilationException & setMessage(String str)     { _message =            str ; return *this; }
    CompilationException & setMessageLine(String str) { _message = addNewLine(str); return *this; }

    CompilationException & appendMessage(String str)     { _message.append(           str ); return *this; }
    CompilationException & appendMessageLine(String str) { _message.append(addNewLine(str)); return *this; }

    const Compiler *_compiler;
    CompilerReturnCode _result;
    CreateLocation _location;
    String _message;

protected:
    String addNewLine(String s) const { return s + String("\n"); }
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(COMPILER_INCL)
