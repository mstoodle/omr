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

#include "Compiler.hpp"
#include "KindService.hpp"
#include "String.hpp"

// Uncomment next line to have the service output the kind ids and vectors to cout as they are assigned
// #define DEBUG_KINDS

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(KindService, Compiler)

KindServiceID KindService::kindServiceID = 0;
const KindService::Kind KindService::NoKind;
const KindService::Kind KindService::AnyKind;

KindService::KindService()
    : Allocatable()
    , _mem("KindServiceAllocator")
    , _id(kindServiceID++)
    , _nextKind(NoKind+1)
    , _nameFromKind(NULL, kmem())
    , _kindVectors(NULL, kmem()) {

    BitVector *newVector = BitVector::newVector(kmem(), NoKind, NoKind);
    assert(newVector != NULL);
    _kindVectors.assign(NoKind, newVector);
    String *noKindName = new (kmem()) String(kmem(), "NoKind");
    _nameFromKind.assign(NoKind, noKindName);
    _kindFromNameMap.insert({*noKindName, NoKind});

    assignKind(NoKind, String("AnyKind"));
}

KindService::~KindService() {
    // need to clear allocated strings and bitvectors
}

static void printBits(BitVector::ForwardIterator &it, BitIndex nextOne=0) {
    if (!it.hasItem())
        return;
    BitIndex oneBit = it.item();
    it++;
    printBits(it, oneBit);
    std::cout << "1";
    while (oneBit != 0 && --oneBit > nextOne)
        std::cout << "0";
}

KindService::Kind
KindService::assignKind(Kind baseKind, String name) {
    // make sure name isn't already in use, if so return its id
    auto found = _kindFromNameMap.find(name);
    if (found != _kindFromNameMap.end()) {
        return found->second;
    }
 
    // Need to create a new kind vector and associate woth the next KindID

    // first though, base kind had better exist
    assert(_kindVectors.exists(baseKind));

    Kind kind = _nextKind++;
    BitVector *newVector = BitVector::newVector(kmem(), kind, kind);
    assert(newVector != NULL);
    BitVector *baseVector = _kindVectors[baseKind];
    *newVector |= *baseVector;

    String *kindName = new (kmem()) String(kmem(), name);
    _kindVectors.assign(kind, newVector);
    _kindFromNameMap.insert({*kindName, kind});
    _nameFromKind.assign(kind, kindName);
    
    #if DEBUG_KINDS
    std::cout << "Kind " << kindName->c_str() << " allocated with kindID " << (size_t) kind << "\n\t";
    BitIndex b=0;
    auto it = newVector->iterator();
    std::cout << "\t";
    printBits(it);
    std::cout << "\n";
    #endif

    return kind;
}

String
KindService::getName(Kind kind) {
    if (_nameFromKind.exists(kind)) {
        return *_nameFromKind[kind];
    }
    return "";
}

bool
KindService::isExactMatch(Kind matchee, Kind matcher) {
    assert(_kindVectors.exists(matchee) && _kindVectors.exists(matcher));
    return (_kindVectors[matchee]->isExactMatch(*_kindVectors[matcher]));
}

bool
KindService::isMatch(Kind matchee, Kind matcher) {
    assert(_kindVectors.exists(matchee) && _kindVectors.exists(matcher));
    return (_kindVectors[matchee]->isMatch(*_kindVectors[matcher]));
}

} // namespace JB2
} // namespace OMR
