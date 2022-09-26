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

#ifndef LITERALDICTIONARY_INCL
#define LITERALDICTIONARY_INCL


#include <stdint.h>
#include <map>
#include <vector>
#include "IDs.hpp"
#include "Iterator.hpp"
#include "typedefs.hpp"

namespace OMR {
namespace JitBuilder {

class Compiler;
class DebugDictionary;
class DynamicType;
class Extension;
class OperationBuilder;

class LiteralDictionary {
    friend class Compilation;
    friend class DebugDictionary;
    friend class DynamicType;
    friend class Extension;
    friend class OperationBuilder;

public:
    LiteralDictionary(Compilation *comp);
    LiteralDictionary(Compilation *comp, std::string name);
    LiteralDictionary(Compilation *comp, std::string name, LiteralDictionary *linkedTypes);
    virtual ~LiteralDictionary();

    LiteralIterator LiteralsBegin() const { return LiteralIterator(_literals); }
    LiteralIterator LiteralsEnd() const { return LiteralIterator(); }

    Literal *LookupLiteral(LiteralID id);
    void RemoveLiteral(Literal *literal);

    LiteralDictionaryID id() const { return _id; }
    std::string name() const { return _name; }
    bool hasLinkedDictionary() const { return _linkedDictionary != NULL; }
    LiteralDictionary *linkedDictionary() { return _linkedDictionary; }

    void write(TextWriter &w);

protected:
    void addNewLiteral(Literal *literal);
    Literal *registerLiteral(LOCATION, const Type *type, const LiteralBytes *value);

    LiteralDictionaryID _id;
    Compilation * _comp;
    std::string _name;
    LiteralVector _literals;
    LiteralVector _ownedLiterals;
    std::map<const Type *,LiteralVector *> _literalsByType;
    LiteralID _nextLiteralID;
    LiteralDictionary * _linkedDictionary;
    };

} // namespace JitBuilder
} // namespace OMR

#endif // defined(LITERALDICTIONARY_INCL)

