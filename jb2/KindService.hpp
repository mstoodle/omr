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

#ifndef KINDSERVICE_INCL
#define KINDSERVICE_INCL

#include <cassert>
#include <cstddef>
#include <map>
#include "Allocatable.hpp"
#include "AllocatorRaw.hpp"
#include "Array.hpp"
#include "BitVector.hpp"
#include "IDs.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {


class KindService : public Allocatable {
    JBALLOC_NO_DESTRUCTOR_(KindService)

public:
    typedef uint64_t Kind;
    KindService();
    virtual ~KindService();

    const static Kind NoKind=0;
    const static Kind AnyKind=1;

    String getName(Kind k);

    Kind assignKind(Kind baseKind, String name);
    bool isExactMatch(Kind matchee, Kind matcher);
    bool isMatch(Kind matchee, Kind matcher);

protected:
    Allocator *kmem() { return &_mem; }

    AllocatorRaw _mem;
    KindServiceID _id;
    Kind _nextKind;
    std::map<String,Kind> _kindFromNameMap;
    Array<String *> _nameFromKind;
    Array<BitVector *> _kindVectors;

    static KindServiceID kindServiceID;
};


// Create a new kind service category typedef
#define KINDTYPE(name) name ## Kind
#define KINDSERVICE_CATEGORY(name) typedef KindService::Kind KINDTYPE(name)


// Use this line in the base class to add support for the new kind service category of the given name
// Usually put at the end of the class declaration in the header since it may modify the apparent
// access type (to protected)
#define BASECLASS_KINDSERVICE_DECL_CONST_ONLY(name) \
public: \
    virtual KINDTYPE(name) kind() const { return _kind; } \
    template<typename T> bool isExactKind() const { \
        return kindService.isExactMatch(_kind, T::get ## name ## ClassKind()); \
    } \
    template<typename T> bool isKind() const { \
        return kindService.isMatch(_kind, T::get ## name ## ClassKind()); \
    } \
    template<typename T> const T *refine() const { \
        assert(isKind<const T>()); \
        return static_cast<const T *>(this); \
    } \
    const KINDTYPE(name) _kind; \
    static const name ## Kind get ## name ## ClassKind(); \
protected: \
    static KindService kindService; \
    static KINDTYPE(name) name ## BaseClassKind; \
    static bool kindRegistered

#define BASECLASS_KINDSERVICE_DECL(name) \
public: \
    template<typename T> T *refine() { \
        assert(isKind<T>()); \
        return static_cast<T *>(this); \
    } \
    BASECLASS_KINDSERVICE_DECL_CONST_ONLY(name)


#define SUBCLASS_KINDSERVICE_DECL(base,name) \
public: \
    static const KINDTYPE(base) get ## base ## ClassKind(); \
protected: \
    static KINDTYPE(base) name ## base ## Kind; \
    static bool kindRegistered

// Use this line in the base class to add support for the new kind service category of the given name
// Usually put at the top of the cpp file and assumes that the base class is "abstract" i.e. it has NoKind
#define BASECLASS_KINDSERVICE_IMPL(name) \
    KindService name::kindService; \
    KINDTYPE(name) name::name ## BaseClassKind=KindService::NoKind; \
    bool name::kindRegistered = false; \
    const name ## Kind \
    name::get ## name ## ClassKind() { \
        if (!kindRegistered) { \
            name ## BaseClassKind = KindService::NoKind; \
            kindRegistered = true; \
        } \
        return name ## BaseClassKind; \
    }

#define BASECLASS_KINDINIT(value) _kind(value)

#define SUBCLASS_KINDSERVICE_IMPL(name,string,super,base) \
    KINDTYPE(base) name::name ## base ## Kind=KindService::NoKind; \
    bool name::kindRegistered = false; \
    const base ## Kind \
    name::get ## base ## ClassKind() { \
        if (!name::kindRegistered) { \
            name::name ## base ## Kind = base::kindService.assignKind(super::get ## base ## ClassKind(), string); \
            name::kindRegistered = true; \
        } \
        return name::name ## base ## Kind; \
    }

#define KIND(base) (get ## base ## ClassKind())
#define CLASSKIND(name,base) (name::get ## base ## ClassKind())
#define KINDINIT(base) _kind(KIND(base))


} // namespace JB2
} // namespace OMR

#endif // defined(KINDSERVICE_INCL)
