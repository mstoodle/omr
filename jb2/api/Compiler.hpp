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

#include <cassert>
#include <exception>
#include <list>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>
#include "IDs.hpp"
#include "CreateLoc.hpp"
#include "typedefs.hpp"

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

    typedef std::vector<Pass *> PassChain;
    typedef std::map<PassID, PassChain> PassRegistry;

public:
    Compiler(std::string name, Config *config=NULL, Compiler *parent=NULL);
    virtual ~Compiler();

    CompilerID id() const { return _id; }
    std::string name() const { return _name; }
    Config *config() const { return _config; }
    Compiler *parent() const { return _parent; }
    TypeDictionary *dict() const { return _dict; }

    ExtensionID getExtensionID() { return _nextExtensionID++; }
    template<typename T>
    T *loadExtension(LOCATION, const SemanticVersion *version=NULL, std::string name=T::NAME) {
        return static_cast<T *>(internalLoadExtension(PASSLOC, version, name));
    }
    template<typename T>
    T *lookupExtension(std::string name=T::NAME) {
        return static_cast<T *>(internalLookupExtension(name));
    }
    bool validateExtension(std::string name) const;

    PassID lookupPass(std::string name);
    CompilerReturnCode compile(LOCATION, Compilation *comp, StrategyID strategyID);

    const std::string actionName(ActionID a) const {
        assert(a < _nextActionID);
        auto found = _actionNames.find(a);
        assert(found != _actionNames.end());
        return found->second;
    }

    const std::string returnCodeName(CompilerReturnCode c) const {
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

    Extension *internalLoadExtension(LOCATION, const SemanticVersion * version, std::string name);
    Extension *internalLookupExtension(std::string name);
    Strategy * lookupStrategy(StrategyID id);

    void extensionCouldNotLoad(LOCATION, std::string name, char *dlerrorMsg);
    void extensionHasNoCreateFunction(LOCATION, Extension *ext, std::string name, char *dlerrorMsg);
    void extensionCouldNotCreate(LOCATION, std::string name);
    void extensionVersionMismatch(LOCATION, std::string name, const SemanticVersion *version, const SemanticVersion *extensionVersion);

    void notifyRecompile(CompileUnit *unit, CompiledBody *oldBody, CompiledBody *newBody, StrategyID strategy=NoStrategy);

    uint64_t _eyeCatcher;
    CompilerID _id;
    std::string _name;
    bool _myConfig;
    Config *_config;
    Compiler *_parent;

    JB1 *_jb1;

    ExtensionID _nextExtensionID;
    std::map<std::string, Extension *> _extensions;

    ActionID assignActionID(std::string name);
    ActionID _nextActionID;
    std::map<ActionID, std::string> _actionNames;

    PassID _nextPassID;
    std::map<std::string, PassID> _registeredPassNames;
    PassRegistry _passRegistry;

    CompilationID getCompilationID() { return this->_nextCompilationID++; }
    CompilationID _nextCompilationID;

    CompileUnitID getCompiledBodyID() { return this->_nextCompiledBodyID++; }
    CompileUnitID _nextCompiledBodyID;

    CompileUnitID getCompileUnitID() { return this->_nextCompileUnitID++; }
    CompileUnitID _nextCompileUnitID;

    ContextID getContextID() { return _nextContextID++; }
    ContextID _nextContextID;

    CompilerReturnCode assignReturnCode(std::string name);
    CompilerReturnCode _nextReturnCode;
    std::map<CompilerReturnCode, std::string> _returnCodeNames;

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
        , _message(std::string("CompilationException")) {
    }

    CompilerReturnCode result() const { return _result; }
    std::string resultString() const { return _compiler->returnCodeName(_result); }

    std::string location() const     { return            _location.to_string() ; }
    std::string locationLine() const { return addNewLine(_location.to_string()); }

    std::string message() const { return _message; }

    CompilationException & setMessage(std::string str)     { _message =            str ; return *this; }
    CompilationException & setMessageLine(std::string str) { _message = addNewLine(str); return *this; }

    CompilationException & appendMessage(std::string str)     { _message.append(           str ); return *this; }
    CompilationException & appendMessageLine(std::string str) { _message.append(addNewLine(str)); return *this; }

    const Compiler *_compiler;
    CompilerReturnCode _result;
    CreateLocation _location;
    std::string _message;

protected:
    std::string addNewLine(std::string s) const { return s + std::string("\n"); }
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(COMPILER_INCL)
