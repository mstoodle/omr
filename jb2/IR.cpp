/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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

#include "AddonIR.hpp"
#include "Builder.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "CompileUnit.hpp"
#include "Config.hpp"
#include "Context.hpp"
#include "CoreExtension.hpp"
#include "Extension.hpp"
#include "ExtensionIRWriter.hpp"
#include "IDSpaceService.hpp"
#include "IR.hpp"
#include "IRDeserializer.hpp"
#include "IRSerializer.hpp"
#include "IRCloner.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "Scope.hpp"
#include "SemanticVersion.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(IR, Compilation)
SUBCLASS_KINDSERVICE_IMPL(IR,"IR",Extensible,Extensible)

IR::IR(Allocator *a, Compiler *compiler)
    : Extensible(a, compiler->primordialExtension(), KIND(Extensible))
    , _id(compiler->getIRID())
    , _nextBuilderID(NoBuilder+1)
    , _nextContextID(NoContext+1)
    , _nextDictionaryID(NoDictionary+1)
    , _nextEntryPointID(NoEntryPoint+1)
    , _nextLiteralID(NoLiteral+1)
    , _nextLocationID(NoLocation+1)
    , _nextOperationID(NoOperation+1)
    , _nextScopeID(NoScope+1)
    , _nextSymbolID(NoSymbol+1)
    , _nextTransformationID(NoTransformation+1)
    , _nextTypeID(NoTypeID+1)
    , _nextValueID(NoValue+1)
    , _compiler(compiler)
    , _unit(NULL)
    , _mem(a)
    , _scope(NULL)
    , _context(NULL)
    , _typedict(new (a) TypeDictionary(a, this, "Compiler Type Dictionary"))
    , _litdict(new (a) LiteralDictionary(a, this, "Compiler Literal Dictionary"))
    , _symdict(new (a) SymbolDictionary(a, this, "Compiler Symbol Dictionary"))
    , _builders(NULL, a)
    , _locations(NULL, a)
    , NoType(new (a) NoTypeType(MEM_LOC(a), compiler->coreExt(), this, NoTypeID)) {

    notifyCreation(KIND(Extensible));
}

IR::IR(Allocator *a, const IR *source, IRCloner *cloner)
    : Extensible(a, source->ext(), KIND(Extensible))
    , _id(source->compiler()->getIRID())
    , _nextBuilderID(source->_nextBuilderID)
    , _nextContextID(source->_nextContextID)
    , _nextDictionaryID(source->_nextDictionaryID)
    , _nextEntryPointID(source->_nextEntryPointID)
    , _nextLiteralID(source->_nextLiteralID)
    , _nextLocationID(source->_nextLocationID)
    , _nextOperationID(source->_nextOperationID)
    , _nextScopeID(source->_nextScopeID)
    , _nextSymbolID(source->_nextSymbolID)
    , _nextTransformationID(source->_nextTransformationID)
    , _nextTypeID(source->_nextTypeID)
    , _nextValueID(source->_nextValueID)
    , _compiler(source->compiler())
    , _unit(source->unit())
    , _mem(a)
    , _scope(NULL)
    , _context(NULL)
    , _litdict(NULL)
    , _symdict(NULL)
    , _typedict(NULL)
    , _builders(NULL, a)
    , _locations(NULL, a) {

    // make sure no IR is created before this is done
    cloner->setClonedIR(this);

    // types first because everything else depends on types
    _typedict = cloner->clonedTypeDictionary(source->typedict());
    _typedict->cloneConstFrom(source->typedict(), cloner);

    // then literal
    _litdict = cloner->clonedLiteralDictionary(source->litdict());
    _litdict->cloneFrom(source->litdict(), cloner);

    // then symbols
    _symdict = cloner->clonedSymbolDictionary(source->symdict());
    _symdict->cloneFrom(source->symdict(), cloner);

    // having cloned Symbol, we can clone the Context
    _context = cloner->clonedContext(source->context<Context>());

    // we can now clone Scopes which clone Builders, Operations and Values which depend on Literals, Symbols, and Types
    _scope = cloner->clonedScope(source->scope<Scope>());

    // make sure NoType has been cloned
    this->NoType = cloner->clonedType(source->NoType)->template refine<const NoTypeType>();

    // do this last so that any elements these addons might create have already been
    // cloned inside other things liked Dictionaries (which retains id's consistently)
    // specifically do not call notifyCreation() because it won't have cloner
    // ...so it might duplicate IR elements as it replicates Addons
    if (source->addons() != NULL) {
        for (auto it = source->addons()->iterator(); it.hasItem(); it++) {
            Addon *sourceAddon = it.item();
            AddonIR *addon = sourceAddon->refine<AddonIR>();
            AddonIR *clonedAddon = addon->clone(a, cloner);
            this->attach(clonedAddon);
        }
    }

}

IR::~IR() {
    for (auto it=_builders.fwdIterator(false, true);it.hasItem(); it++) {
        Builder *b = it.item();
        delete b;
    }

    delete _symdict;
    delete _litdict;
    delete _typedict;

    // scope and context objects may not be (but probably are) dynamically allocated
    if (_scope != NULL && _scope->allocator())
        delete _scope;
    if (_context != NULL && _context->allocator())
        delete _context;
}

void
IR::addInitialBuildersToWorklist(BuilderList & worklist) {
    _scope->addInitialBuildersToWorklist(worklist);
}

Literal *
IR::registerLiteral(LOCATION, const Type *type, const LiteralBytes *value) {
    return _litdict->registerLiteral(PASSLOC, type, value);
}

void
IR::registerBuilder(Builder *b) {
    _builders.push_back(b);
}

void
IR::unregisterBuilder(Builder *b) {
    auto it = _builders.find(b);
    if (it.hasItem())
        _builders.remove(it);
}

void
IR::registerLocation(Location *location) {
    _locations.push_back(location);
}

bool
IR::prepare(LOCATION, Compilation *comp) {
    if (unit()->buildContext(PASSLOC, comp, _scope, _context) == false)
        return false;

    return unit()->buildIL(PASSLOC, comp, _scope, _context);
}

bool
IR::build(LOCATION, Compilation *comp) {
    return unit()->buildIL(PASSLOC, comp, _scope, _context);
}

IR *
IR::clone(Allocator *mem) const {
    IRCloner *cloner = new (mem) IRCloner(mem, ext());
    IR *clonedIR = new (mem) IR(mem, this, cloner);
    delete cloner;
    return clonedIR;
}

IR *
IR::read(Allocator *mem, Compiler *compiler, IRDeserializer & d) {
    IR *ir = new (mem) IR(mem, compiler);
    if (!ir->read(d)) {
        delete ir;
        return NULL;
    }
    return ir;
}

bool
IR::read(IRDeserializer & d) {
    d.beginSection("ir");

    {
        d.beginSection("extensions");
        uint32_t count = d.readCount();

        for (uint32_t i = 0; i < count; ++i) {
            uint64_t fExtID = d.readExtID();
            String name;
            d.readString(name);

            uint16_t major = 0, minor = 0, patch = 0;
            d.readUInt16(major);
            d.readUInt16(minor);
            d.readUInt16(patch);

            String preRelease, buildMetadata;
            d.readString(preRelease);
            d.readString(buildMetadata);

            SemanticVersion semver(major, minor, patch, preRelease, buildMetadata);

            Extension *ext = _compiler->lookupExtensionByName(name);
            if (ext == NULL) {
                ext = _compiler->loadExtensionByName(LOC, name, &semver);
            }
            if (ext == NULL) {
                return false;
            }

            d.mapExtension(fExtID, ext);

            // Read id_spaces
            d.beginSection("id_spaces");
            uint32_t spaceCount = d.readCount();
            for (uint32_t s = 0; s < spaceCount; ++s) {
                String tag;
                d.readString(tag);
                IDSpaceService::service.idSpaceFor(tag);
            }
            d.endSection("id_spaces");

            // Read actions
            d.beginSection("actions");
            uint32_t actionCount = d.readCount();
            for (uint32_t a = 0; a < actionCount; ++a) {
                ActionID fActionID = d.readActionID();
                String actName;
                d.readString(actName);
                ActionID liveActionID = _compiler->lookupActionFromName(actName);
                d.mapAction(fActionID, liveActionID);
            }
            d.endSection("actions");
        }

        d.endSection("extensions");
    }

    d.endSection("ir");
    return true;
}

void
IR::write(IRSerializer & s) const {
    IRSerializer::Section irSec(s, "ir");

    {
        IRSerializer::Section extSec(s, "extensions");

        uint32_t count = _compiler->numExtensions();
        s.writeCount(count);

        uint32_t numSpaces = IDSpaceService::service.count();

        for (ExtensionID id = 1; id <= count; ++id) {
            Extension *ext = _compiler->extension(id);
            if (ext != NULL && ext->irWriter() != NULL) {
                ExtensionIRWriter *writer = ext->irWriter();
                writer->write(s);
                for (IDSpaceID space = 1; space <= numSpaces; ++space) {
                    writer->writeElementsForIDSpace(s, space);
                }
            }
        }
    }
}

void
IR::log(Compilation *comp, TextLogger &lgr) const {
    if (unit()) {
        lgr.irSectionBegin("ir", "ir", id(), kind(), unit()->createLoc()->functionName(mem()));
        unit()->log(lgr);
    } else {
        lgr.irSectionBegin("ir", "ir", id(), kind(), "irPrototype");
    }

    typedict()->log(lgr);
    litdict()->log(lgr);
    symdict()->log(lgr);

    if (_context != NULL)
        _context->log(lgr);

    if (_scope != NULL)
        _scope->log(lgr);

    lgr.irSectionEnd();
}

} // namespace JB2
} // namespace OMR
