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

#include <dlfcn.h>
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Config.hpp"
#include "Extension.hpp"
#include "JB1.hpp"
#include "JB1CodeGenerator.hpp"
#include "Pass.hpp"
#include "SemanticVersion.hpp"
#include "Strategy.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

CompilerID Compiler::nextCompilerID = 1; // 0 is reserved
EyeCatcher Compiler::EYE_CATCHER_COMPILER=0xAABBCCDDDDCCBBAA;

Compiler::Compiler(String name, Config *config, Compiler *parent)
    : _eyeCatcher(EYE_CATCHER_COMPILER)
    , _id(nextCompilerID++)
    , _name(name)
    , _myConfig(config == NULL)
    , _config(config != NULL ? config : new Config())
    , _parent(parent)
    , _jb1(JB1::instance())
    , _nextExtensionID(NoExtension+1)
    , _extensions()
    , _nextActionID(NoAction+1)
    , _actionNames()
    , _nextPassID(NoPass+1)
    , _registeredPassNames()
    , _passRegistry()
    , _nextCompilationID(NoCompilation)
    , _nextCompileUnitID(NoCompileUnit)
    , _nextCompiledBodyID(NoCompiledBody)
    , _nextContextID(NoContext)
    , _nextReturnCode(0)
    , _returnCodeNames()
    , _nextStrategyID(NoStrategy+1)
    , _strategies()
    , _nextTypeID(NoTypeID+1)
    , _types()
    , _nextTypeDictionaryID(0)
    , _target(NULL)
    , _compiler(NULL)
    , _dict(new TypeDictionary(this, name + "::root"))
    , _errorCondition(NULL)
    , CompileSuccessful(assignReturnCode("CompileSuccessful"))
    , CompileNotStarted(assignReturnCode("CompileNotStarted"))
    , CompileFailed(assignReturnCode("CompileFailed"))
    , CompileFail_UnknownStrategyID(assignReturnCode("CompileFail_UnknownStrategy"))
    , CompileFail_IlGen(assignReturnCode("CompileFail_IlGen"))
    , CompileFail_TypeMustBeReduced(assignReturnCode("CompileFail_TypeMustBeReduced"))
    , CompilerError_Extension_CouldNotLoad(assignReturnCode("CompilerError_Extension_CouldNotLoad"))
    , CompilerError_Extension_HasNoCreateFunction(assignReturnCode("CompilerError_Extension_HasNoCreateFunction"))
    , CompilerError_Extension_CouldNotCreate(assignReturnCode("CompilerError_Extension_CouldNotCreate"))
    , CompilerError_Extension_VersionMismatch(assignReturnCode("CompilerError_Extension_VersionMismatch")) {

    _jb1->initialize();

    Strategy *jb1cgStrategy = new Strategy(this, "jb1cg");
    Pass *jb1cg = new JB1CodeGenerator(this);
    jb1cgStrategy->addPass(jb1cg);
    jb1cgStrategyID = jb1cgStrategy->id();

    // has to be last since Extension's constructor will use this object
    if (parent == NULL)
        addExtension(new Extension(LOC, this));
}

Compiler::~Compiler() {
    _jb1->shutdown();
    if (_dict != NULL)
        delete _dict;
    if (_myConfig && _config != NULL)
        delete _config;
    for (auto it = _extensions.begin();it != _extensions.end();it++) {
        Extension *ext = it->second;
        delete ext;
     }
    _extensions.clear();
    if (_errorCondition)
        delete _errorCondition;
}

extern "C" {
    typedef Extension * (*CreateFunction)(LOCATION, Compiler *);
}

Extension *
Compiler::internalLoadExtension(LOCATION, const SemanticVersion *version, String name) {
    Extension *ext = internalLookupExtension(name);
    if (ext) {
        if (version == NULL || ext->semver()->isCompatibleWith(*version))
            return ext;
        extensionVersionMismatch(PASSLOC, name, version, ext->semver());
        return NULL;
    }

    String soname = String("lib") + name + String(".so");
    void *handle = dlopen(soname.c_str(), RTLD_LAZY);
    if (!handle) {
        extensionCouldNotLoad(LOC, soname, dlerror());
        return NULL;
    }

    CreateFunction create = (CreateFunction) dlsym(handle, "create");
    char * error;
    if ((error = dlerror()) != NULL) {
        dlclose(handle);
        extensionHasNoCreateFunction(LOC, ext, soname, error);
        return NULL;
    }

    if (create == NULL) {
        dlclose(handle);
        extensionHasNoCreateFunction(LOC, ext, soname, dlerror());
        return NULL;
    }

    ext = create(LOC, this);
    if (ext == NULL) {
        dlclose(handle);
        extensionCouldNotCreate(LOC, soname);
        return NULL;
    }

    if (version == NULL || ext->semver()->isCompatibleWith(*version)) {
        addExtension(ext);
        return ext;
    }

    const SemanticVersion * extVer = ext->semver();
    delete ext;
    dlclose(handle);
    extensionVersionMismatch(LOC, soname, version, extVer);
    return NULL;
}

void
Compiler::addExtension(Extension *ext) {
    this->_extensions.insert({ext->name(),ext});
}

bool
Compiler::validateExtension(String name) const {
    auto it = _extensions.find(name);
    if (it != _extensions.end())
        return true;
    if (_parent)
        return _parent->validateExtension(name);
    return false;
}

Extension *
Compiler::internalLookupExtension(String name) {
    auto it = _extensions.find(name);
    if (it == _extensions.end()) {
        if (_parent != NULL)
            return _parent->internalLookupExtension(name);
        return NULL;
    }
    assert(it->first == name);
    return it->second;
}

ActionID
Compiler::assignActionID(String name) {
    ActionID id = this->_nextActionID++;
    this->_actionNames.insert({id, name});
    return id;
}

CompilerReturnCode
Compiler::assignReturnCode(String name) {
    CompilerReturnCode rc = this->_nextReturnCode++;
    this->_returnCodeNames.insert({rc, name});
    return rc;
}

PassID
Compiler::addPass(Pass *pass) {
    auto it = this->_registeredPassNames.find(pass->name());
    if (it != this->_registeredPassNames.end())
        return it->second;
    return _nextPassID++;
}

PassID
Compiler::lookupPass(String name) {
    auto it = this->_registeredPassNames.find(name);
    if (it != this->_registeredPassNames.end())
        return it->second;
    return NoPass;
}

StrategyID
Compiler::addStrategy(Strategy *st) {
    StrategyID id = this->_nextStrategyID++;
    this->_strategies.insert({id, st});
    return id;
}

Strategy *
Compiler::lookupStrategy(StrategyID id) {
    auto it = _strategies.find(id);
    if (it == _strategies.end()) {
        if (_parent != NULL)
            return _parent->lookupStrategy(id);
        return NULL;
    }
    return it->second;
}

CompilerReturnCode
Compiler::compile(LOCATION, Compilation *comp, StrategyID strategyID) {
    try {

        bool success = comp->prepareIL(PASSLOC);
        if (!success)
            return CompileFail_IlGen;

        if (strategyID == NoStrategy) // nothing more to do
            return CompileSuccessful;

        Strategy *st = lookupStrategy(strategyID);
        if (!st)
            return CompileFail_UnknownStrategyID;

        return st->perform(comp);

    } catch (CompilationException e) {
        // only if config.verboseErrors()?
        std::cerr << "Location: " << e.locationLine().c_str();
        std::cerr << e._message.c_str();
        return e._result;
    }

    return CompileSuccessful;
}

void
Compiler::extensionCouldNotLoad(LOCATION, String name, char *dlerrorMsg) {
    CompilationException *e = new CompilationException(PASSLOC, this, this->CompilerError_Extension_CouldNotLoad);
    (*e).setMessageLine(String("Extension could not be loaded"))
        .appendMessageLine(String("Library name: ").append(name))
        .appendMessageLine(String("dlerror() reports ").append(String(dlerrorMsg)));
    _errorCondition = e;
}

void
Compiler::extensionHasNoCreateFunction(LOCATION, Extension *ext, String name, char *dlerrorMsg) {
    CompilationException *e = new CompilationException(PASSLOC, this, this->CompilerError_Extension_HasNoCreateFunction);
    (*e).setMessageLine(String("Extension does not have a create() function"))
        .appendMessageLine(String("Library loaded: ").append(name))
        .appendMessageLine(String("dlerror() reports ").append(String(dlerrorMsg)));
    _errorCondition = e;
}

void
Compiler::extensionCouldNotCreate(LOCATION, String name) {
    CompilationException *e = new CompilationException(PASSLOC, this, this->CompilerError_Extension_CouldNotCreate);
    (*e).setMessageLine(String("Extension create() function returned NULL"))
        .appendMessageLine(String("Library loaded: ").append(name));
    _errorCondition = e;
}

void
Compiler::extensionVersionMismatch(LOCATION, String name, const SemanticVersion * version, const SemanticVersion * extensionVersion) {
    CompilationException *e = new CompilationException(PASSLOC, this, this->CompilerError_Extension_VersionMismatch);
    (*e).setMessageLine(String("Extension version mismatch"))
        .appendMessageLine(String("Requested: major ").append(String::to_string(version->major())))
        .appendMessageLine(String("           minor ").append(String::to_string(version->minor())))
        .appendMessageLine(String("           patch ").append(String::to_string(version->patch())))
        .appendMessageLine(String("Loaded:    major ").append(String::to_string(extensionVersion->major())))
        .appendMessageLine(String("           minor ").append(String::to_string(extensionVersion->minor())))
        .appendMessageLine(String("           patch ").append(String::to_string(extensionVersion->patch())));
    _errorCondition = e;
}

void
Compiler::notifyRecompile(CompileUnit *unit, CompiledBody *oldBody, CompiledBody *newBody, StrategyID strategy) {
    // TODO: notify registered Listeners
}

} // namespace JitBuilder
} // namespace OMR
