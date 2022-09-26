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
#include "Pass.hpp"
#include "SemanticVersion.hpp"
#include "Strategy.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

CompilerID Compiler::nextCompilerID = 1; // 0 is reserved

Compiler::Compiler(std::string name, Config *config)
    : _id(nextCompilerID++)
    , _name(name)
    , _jb1(JB1::instance())
    , _config(config)
    , _myConfig(false)
    , _myDict(false)
    , _nextExtensionID(NoExtension+1)
    , _extensions()
    , _nextActionID(NoAction+1)
    , _actionNames()
    , _nextPassID(NoPass+1)
    , _registeredPassNames()
    , _passRegistry()
    , _nextReturnCode(0)
    , _returnCodeNames()
    , _nextStrategyID(NoStrategy+1)
    , _strategies()
    , _nextTypeID(NoType+1)
    , _types()
    , _nextTypeDictionaryID(0)
    , _target(NULL)
    , _compiler(NULL)
    , _dict(new TypeDictionary(this, name + "::root"))
    , CompileSuccessful(assignReturnCode("CompileSuccessful"))
    , CompileNotStarted(assignReturnCode("CompileNotStarted"))
    , CompileFailed(assignReturnCode("CompileFailed"))
    , CompileFail_UnknownStrategyID(assignReturnCode("CompileFail_UnknownStrategy"))
    , CompileFail_IlGen(assignReturnCode("CompileFail_IlGen"))
    , CompileFail_TypeMustBeReduced(assignReturnCode("CompileFail_TypeMustBeReduced")) {

    if (_config == NULL) {
        _config = new Config();
        _myConfig = true;
    }
    _jb1->initialize();
}

Compiler::~Compiler() {
    _jb1->shutdown();
    delete _dict;
    if (_myConfig && _config != NULL)
        delete _config;
    for (auto it = _extensions.begin();it != _extensions.end();it++) {
        Extension *ext = it->second;
        delete ext;
     }
    _extensions.clear();
}

extern "C" {
    typedef Extension * (*CreateFunction)(Compiler *);
}

Extension *
Compiler::internalLoadExtension(std::string name, SemanticVersion *version) {
    Extension *ext = internalLookupExtension(name);
    if (ext) {
        if (version == NULL || ext->semver()->isCompatibleWith(*version))
            return ext;
        return NULL;
    }

    std::string soname = std::string("lib") + name + std::string(".so");
    void *handle = dlopen(soname.c_str(), RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        fputs("\n", stderr);
        return NULL;
    }

    CreateFunction create = (CreateFunction) dlsym(handle, "create");
    char * error;
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
        fputs("\n", stderr);
        dlclose(handle);
        return NULL;
    }

    if (create == NULL) {
        dlclose(handle);
        return NULL;
    }

    ext = create(this);
    if (ext == NULL) {
        dlclose(handle);
        return NULL;
    }

    if (version == NULL || ext->semver()->isCompatibleWith(*version)) {
        addExtension(ext);
	    return ext;
    }

    delete ext;
    dlclose(handle);
    return NULL;
}

void
Compiler::addExtension(Extension *ext) {
    this->_extensions.insert({ext->name(),ext});
}

bool
Compiler::validateExtension(std::string name) const {
    auto it = _extensions.find(name);
    return (it != _extensions.end());
}

Extension *
Compiler::internalLookupExtension(std::string name) {
    auto it = _extensions.find(name);
    if (it == _extensions.end())
        return NULL;
    assert(it->first == name);
    return it->second;
}

ActionID
Compiler::assignActionID(std::string name) {
    ActionID id = this->_nextActionID++;
    this->_actionNames.insert({id, name});
    return id;
}

CompilerReturnCode
Compiler::assignReturnCode(std::string name) {
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
Compiler::lookupPass(std::string name) {
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
    if (it == _strategies.end())
        return NULL;
    return it->second;
}

CompilerReturnCode
Compiler::compile(Compilation *comp, StrategyID strategyID) {
    try {
        bool success = comp->buildIL();
        if (!success)
            return CompileFail_IlGen;

        if (strategyID == NoStrategy)
            return CompileSuccessful;

        Strategy *st = lookupStrategy(strategyID);
        if (!st)
            return CompileFail_UnknownStrategyID;

        return st->perform(comp);
    } catch (CompilationException e) {
        // only if config.verboseErrors()?
        std::cerr << "Location: " << e.locationLine();
        std::cerr << e._message;
        return e._result;
    }

    return CompileSuccessful;
}

} // namespace JitBuilder
} // namespace OMR

