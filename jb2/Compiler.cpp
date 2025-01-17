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

#include <stdio.h>

#include <assert.h>
#include <dlfcn.h>
#include "omrcfg.h"
#include "AllocatorTracker.hpp"
#include "Compilation.hpp"
#include "CompiledBody.hpp"
#include "Compiler.hpp"
#include "Config.hpp"
#include "CoreExtension.hpp"
#include "Extension.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"
#include "Literal.hpp"
#include "Pass.hpp"
#include "SemanticVersion.hpp"
#include "Strategy.hpp"
#include "Symbol.hpp"
#include "TextWriter.hpp"
#include "Type.hpp"

namespace OMR {
namespace JB2 {

CompilerID Compiler::nextCompilerID = 1; // 0 is reserved
EyeCatcher Compiler::EYE_CATCHER_COMPILER=0xAABBCCDDDDCCBBAA;

INIT_JBALLOC(Compiler);

#define CTOR_FIELDS(p,n,cfg,m) \
    , _eyeCatcher(EYE_CATCHER_COMPILER) \
    , _id(nextCompilerID++) \
    , _mallocAllocator() \
    , _baseAllocator(((m) != NULL) ? (m) : (&_mallocAllocator)) \
    , _name(_baseAllocator, n) \
    , _myConfig((cfg) == NULL) \
    , _config(_myConfig ? new (_baseAllocator) Config(_baseAllocator) : (cfg)) \
    , _mem(_config->compilerAllocator(_baseAllocator)) \
    , _parent(p) \
    , _nextExtensionID(NoExtension+1) \
    , _extensions() \
    , _nextActionID(NoAction+1) \
    , _actionNames() \
    , _nextPassID(NoPass+1) \
    , _passNames() \
    , _extensiblesByKind() \
    , _nextContextID(NoContext+1) \
    , _nextExecutorID(NoExecutor+1) \
    , _nextReturnCode(0) \
    , _returnCodeNames() \
    , _nextStrategyID(NoStrategy+1) \
    , _strategies() \
    , _nextIRID(0) \
    , _targetPlatform(NULL) \
    , _compilerPlatform(NULL) \
    , _clientPlatform(NULL) \
    , _coreExtension(NULL) \
    , _primordialExtension(new (_mem) Extension(MEM_LOC(_mem), CLASSKIND(Extension, Extensible), this, "Primordial")) \
    , _textWriters(NULL, _mem) \
    , _errorCondition(NULL) \
    , _irPrototype(new (_mem) IR(_mem, this)) \
    , CompileSuccessful(assignReturnCode("CompileSuccessful")) \
    , CompileNotStarted(assignReturnCode("CompileNotStarted")) \
    , CompileFailed(assignReturnCode("CompileFailed")) \
    , CompileFail_CompilerError(assignReturnCode("CompileFail_CompilerError")) \
    , CompileFail_UnknownStrategyID(assignReturnCode("CompileFail_UnknownStrategy")) \
    , CompileFail_IlGen(assignReturnCode("CompileFail_IlGen")) \
    , CompileFail_TypeMustBeReduced(assignReturnCode("CompileFail_TypeMustBeReduced")) \
    , CompilerError_Extension_CouldNotLoad(assignReturnCode("CompilerError_Extension_CouldNotLoad")) \
    , CompilerError_Extension_HasNoCreateFunction(assignReturnCode("CompilerError_Extension_HasNoCreateFunction")) \
    , CompilerError_Extension_CouldNotCreate(assignReturnCode("CompilerError_Extension_CouldNotCreate")) \
    , CompilerError_Extension_VersionMismatch(assignReturnCode("CompilerError_Extension_VersionMismatch"))


Compiler::Compiler(Allocator *a, String name, Config *config)
    : Allocatable(a)
    CTOR_FIELDS(NULL, name, config, a) {

}

Compiler::Compiler(String name, Config *config)
    : Allocatable()
    CTOR_FIELDS(NULL, name, config, NULL) {

}

Compiler::Compiler(Allocator *a, Compiler *parent, String name, Config *config)
    : Allocatable(a)
    CTOR_FIELDS(parent, name, config, a) {

}

Compiler::Compiler(Compiler *parent, String name, Config *config)
    : Allocatable()
    CTOR_FIELDS(parent, name, config, NULL) {

}

#undef CTOR_FIELDS

Compiler::~Compiler() {
    for (auto it = _extensionsForAddonsByKind.begin(); it != _extensionsForAddonsByKind.end(); it++) {
        List<Extension *> *list = it->second;
        if (list != NULL) {
            delete list;
        }
    }
    _extensionsForAddonsByKind.clear();

    for (auto it =_extensiblesByKind.begin(); it != _extensiblesByKind.end(); it++) {
        List<Extensible *> *kindList = it->second;
        if (kindList != NULL) {
            for (auto it2=kindList->iterator();it2.hasItem();it2++) {
                Extensible *e = it2.item();
                delete e;
            }
            delete kindList;
        }
    }
    _extensiblesByKind.clear();

    for (auto it = _extensions.begin();it != _extensions.end();it++) {
        Extension *ext = it->second;
        delete ext;
     }
    _extensions.clear();

    for (auto it = _strategies.begin();it != _strategies.end();it++) {
        Strategy *st = it->second;
        delete st;
    }
    _strategies.clear();

    delete _irPrototype;

    for (auto wIt = _textWriters.iterator(); wIt.hasItem(); wIt++) {
        TextWriter *w = wIt.item();
        delete w;
    }

    if (_errorCondition != NULL) {
        delete _errorCondition;
        _errorCondition = NULL;
    }

    _config->destructCompilerAllocator(_mem);

    //JB2::report();

    if (_myConfig && _config != NULL)
        delete _config;

}

CoreExtension *
Compiler::coreExt() {
    if (_coreExtension == NULL) {
        if (_parent != NULL) {
            _coreExtension = _parent->lookupExtension<CoreExtension>();
        } else {
            _coreExtension = new (_mem) CoreExtension(MEM_LOC(_mem), this);
        }
        addExtension(_coreExtension);
    }

    return _coreExtension;
}

LiteralDictionary *
Compiler::litdict() const {
    return _irPrototype->litdict();
}

SymbolDictionary *
Compiler::symdict() const {
    return _irPrototype->symdict();
}

TypeDictionary *
Compiler::typedict() const {
    return _irPrototype->typedict();
}

IR *
Compiler::createIR(Allocator *mem) const {
    return _irPrototype->clone(mem);
}

bool
Compiler::platformImplements8bCompares() const {
    #if defined(AARCH64)
        return false;
    #else
        return true;
    #endif
}

bool
Compiler::platformImplements16bCompares() const {
    #if defined(AARCH64)
        return false;
    #else
        return true;
    #endif
}

const char *
Compiler::platformLibrarySuffix() const {
    #if defined(OSX)
    return ".dylib";
    #elif defined(__WINDOWS__)
    return ".dll";
    #else
    return ".so";
    #endif
    
    // ideally, this never happens
    return "";
}

Extension *
Compiler::internalLoadExtension(LOCATION, String name, const SemanticVersion *version) {
    Extension *ext = internalLookupExtension(name);
    if (ext) {
        if (version == NULL || ext->semver()->isCompatibleWith(*version))
            return ext;
        extensionVersionMismatch(PASSLOC, name, version, ext->semver());
        return NULL;
    }

    String soname = String(mem(), "lib") + name + String(mem(), platformLibrarySuffix());
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
    for (auto it=_extensions.begin(); it != this->_extensions.end();it++) {
        assert(!(it->first == ext->name()));
        Extension *other = it->second;
        ext->notifyNewExtension(other);
        other->notifyNewExtension(ext);
    }
    this->_extensions.insert({ext->name(),ext});
    //for (auto it2=_extensions.begin(); it2 != this->_extensions.end();it2++) {
    //}
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

void
Compiler::registerForExtensible(ExtensibleKind kind, Extension *ext) {
    auto it = _extensionsForAddonsByKind.find(kind);
    List<Extension *> *list=NULL;
    if (it != _extensionsForAddonsByKind.end()) {
        list = it->second;
    } else {
        list = new (mem()) List<Extension *>(mem(), mem());
        _extensionsForAddonsByKind.insert({kind, list});
    }
    list->push_back(ext);
}

void
Compiler::createAnyAddons(Extensible *e, KINDTYPE(Extensible) kind) {
    auto list = _extensionsForAddonsByKind.find(kind);
    if (list != _extensionsForAddonsByKind.end()) {
        for (auto it=list->second->iterator(); it.hasItem(); it++) {
            Extension *ext = it.item();
            ext->createAddon(e);
        }
    }
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

void
Compiler::registerExtensible(Extensible *e, KINDTYPE(Extensible) kind) {
    auto it = _extensiblesByKind.find(kind);
    List<Extensible *> *kindList = NULL;
    if (it == _extensiblesByKind.end()) {
        kindList = new (_mem) List<Extensible *>(_mem, _mem);
        _extensiblesByKind.insert({kind, kindList});
    } else {
        kindList = it->second;
    }
    kindList->push_back(e);
}

PassID
Compiler::addPass(Pass *pass) {
    #if 0
    PassID id=NoPass;
    auto it = this->_passNames.find(pass->name());
    if (it != this->_passNames.end())
        id = it->second;
    else {
        id = _nextPassID++;
        this->_passNames.insert({pass->name(), id});
    }
    #endif

    PassID id = _nextPassID++;
    this->_passNames.insert({pass->name(), id});

    //registerExtensible(pass, CLASSKIND(Pass, Extensible));

    return id;
}

PassID
Compiler::lookupPass(String name) {
    auto it = this->_passNames.find(name);
    if (it != this->_passNames.end())
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

TextWriter *
Compiler::textWriter(TextLogger & lgr) {
    for (auto it=_textWriters.iterator(); it.hasItem(); it++) {
        TextWriter *w = it.item();
        if (&(w->logger()) == &lgr)
            return w;
    }

    TextWriter *w = new (_mem) TextWriter(_mem, this, lgr);
    _textWriters.push_front(w);
    return w;
}

CompiledBody *
Compiler::compile(LOCATION, Compilation *comp, StrategyID strategyID) {
    CompileUnit *unit = comp->unit();
    CompiledBody *body = new (_mem) CompiledBody(_mem, unit, strategyID);
    if (_errorCondition != NULL) // error should be clear at the start
        return body->setReturnCode(CompileFail_CompilerError);

    if (config()->tracePrototypeIR()) {
        AllocatorRaw raw;
        irPrototype()->log(comp, *comp->logger());
    }

    CompilerReturnCode rc = CompileSuccessful;
    try {
        bool success = comp->prepareIL(PASSLOC);

        // From this point, cannot return early without freeing the IL
        // Below is designed to always fall through to the end where IL is freed

        if (success) {
            if (config()->traceBuildIL()) {
                AllocatorRaw raw;
                TextWriter *writer = new (&raw) TextWriter(&raw, this, *comp->logger());
                writer->perform(comp);
                delete writer;
            }

            if (strategyID != NoStrategy) {
                Strategy *st = lookupStrategy(strategyID);
                if (st != NULL) {
                    rc = st->perform(comp);
                    if (rc == CompileSuccessful) {
                        comp->ir()->scope<Scope>()->saveEntries(body);
                    }
                } else {
                    rc = CompileFail_UnknownStrategyID;
                }
            }
        } else {
            rc = CompileFail_IlGen;
        }
    } catch (CompilationException e) {
        if (config()->verboseErrors()) {
            std::cerr << "Location: " << e.locationLine().c_str();
            std::cerr << e.message().c_str();
        }
        rc = e.result();
    }

    comp->freeIL(PASSLOC);
    unit->saveCompiledBody(body, strategyID); // body will be freed when unit is freed
    return body->setReturnCode(rc);
}

void
Compiler::clearErrorCondition() {
    if (_errorCondition != NULL) {
        delete _errorCondition;
        _errorCondition = NULL;
    }
}

void
Compiler::extensionCouldNotLoad(LOCATION, String name, char *dlerrorMsg) {
    CompilationException *e = new (_mem) CompilationException(MEM_PASSLOC(_mem), this, this->CompilerError_Extension_CouldNotLoad);
    (*e).setMessageLine(String(_mem, "Extension could not be loaded"))
        .appendMessageLine(String(_mem, "Library name: ").append(name))
        .appendMessageLine(String(_mem, "dlerror() reports ").append(String(dlerrorMsg)));
    _errorCondition = e;
}

void
Compiler::extensionHasNoCreateFunction(LOCATION, Extension *ext, String name, char *dlerrorMsg) {
    CompilationException *e = new (_mem) CompilationException(MEM_PASSLOC(_mem), this, this->CompilerError_Extension_HasNoCreateFunction);
    (*e).setMessageLine(String(_mem, "Extension does not have a create() function"))
        .appendMessageLine(String(_mem, "Library loaded: ").append(name))
        .appendMessageLine(String(_mem, "dlerror() reports ").append(String(dlerrorMsg)));
    _errorCondition = e;
}

void
Compiler::extensionCouldNotCreate(LOCATION, String name) {
    CompilationException *e = new (_mem) CompilationException(MEM_PASSLOC(_mem), this, this->CompilerError_Extension_CouldNotCreate);
    (*e).setMessageLine(String(_mem, "Extension create() function returned NULL"))
        .appendMessageLine(String(_mem, "Library loaded: ").append(name));
    _errorCondition = e;
}

void
Compiler::extensionVersionMismatch(LOCATION, String name, const SemanticVersion * version, const SemanticVersion * extensionVersion) {
    CompilationException *e = new (_mem) CompilationException(MEM_PASSLOC(_mem), this, this->CompilerError_Extension_VersionMismatch);
    (*e).setMessageLine(String(_mem, "Extension version mismatch"))
        .appendMessageLine(String(_mem, "Requested: major ").append(String::to_string(_mem, version->major())))
        .appendMessageLine(String(_mem, "           minor ").append(String::to_string(_mem, version->minor())))
        .appendMessageLine(String(_mem, "           patch ").append(String::to_string(_mem, version->patch())))
        .appendMessageLine(String(_mem, "Loaded:    major ").append(String::to_string(_mem, extensionVersion->major())))
        .appendMessageLine(String(_mem, "           minor ").append(String::to_string(_mem, extensionVersion->minor())))
        .appendMessageLine(String(_mem, "           patch ").append(String::to_string(_mem, extensionVersion->patch())));
    _errorCondition = e;
}

void
Compiler::notifyRecompile(CompileUnit *unit, CompiledBody *oldBody, CompiledBody *newBody, StrategyID strategy) {
    // TODO: notify registered Listeners
}

CompilationException::CompilationException(Allocator *a,
                                           LOCATION,
                                           Compiler *compiler,
                                           CompilerReturnCode result)
    : std::exception(), Allocatable(a)
    , _compiler(compiler)
    , _result(result)
    , _location(PASSLOC)
    , _message(String(compiler->mem(), "CompilationException")) {
}

CompilationException::CompilationException(LOCATION,
                                           Compiler *compiler,
                                           CompilerReturnCode result)
    : std::exception(), Allocatable()
    , _compiler(compiler)
    , _result(result)
    , _location(PASSLOC)
    , _message(String(compiler->mem(), "CompilationException")) {
}

CompilationException::~CompilationException() {

}
} // namespace JB2
} // namespace OMR
