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
#include <string>
#include <map>
#include "IDs.hpp"

namespace OMR {
namespace JitBuilder {


class KindService {
public:
    typedef uint64_t Kind;
    KindService()
        : _id(kindServiceID++)
        , _nextKind(AnyKind+1) {
    }

    const static Kind NoKind=0;
    const static Kind AnyKind=1;

    Kind getNextKind(Kind k);

    Kind assignKind(Kind baseKind, std::string name);
    bool isExactMatch(Kind matchee, Kind matcher) {
        return (matchee == matcher);
    }
    bool isMatch(Kind matchee, Kind matcher) {
        return ((matchee & matcher) == matcher);
    }

protected:

    KindServiceID _id;
    Kind _nextKind;
    std::map<std::string,Kind> _kindFromNameMap;
    std::map<Kind,std::string> _nameFromKindMap;

    static KindServiceID kindServiceID;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(KINDSERVICE_INCL)

