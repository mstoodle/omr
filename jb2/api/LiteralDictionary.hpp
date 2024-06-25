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


#include <map>
#include "common.hpp"
#include "CreateLoc.hpp"
#include "ExtensibleIR.hpp"

namespace OMR {
namespace JitBuilder {

class Compiler;
class DebugDictionary;
class DynamicType;
class Extension;
class IR;
class OperationBuilder;
class TextLogger;

class LiteralDictionary : public ExtensibleIR {
    JBALLOC_(LiteralDictionary)

    friend class Compiler;
    friend class Compilation;
    friend class DebugDictionary;
    friend class DynamicType;
    friend class Extension;
    friend class IR;
    friend class IRCloner;
    friend class Literal;
    friend class OperationBuilder;

public:
    DYNAMIC_ALLOC_ONLY(LiteralDictionary, Compiler *compiler);
    DYNAMIC_ALLOC_ONLY(LiteralDictionary, Compiler *compiler, String name);
    DYNAMIC_ALLOC_ONLY(LiteralDictionary, Compiler *compiler, String name, LiteralDictionary *linkedTypes);

    LiteralListIterator literalIterator() const { return _literals.iterator(); }

    Literal *LookupLiteral(LiteralID id);
    void RemoveLiteral(Literal *literal);

    LiteralDictionaryID id() const { return _id; }
    const String & name() const { return _name; }
    bool hasLinkedDictionary() const { return _linkedDictionary != NULL; }
    LiteralDictionary *linkedDictionary() const { return _linkedDictionary; }

    void log(TextLogger &lgr);

protected:
    LiteralDictionary(Allocator *a, const LiteralDictionary *source, IRCloner *cloner);
    virtual void logContents(TextLogger &lgr);

    LiteralID getLiteralID() { return _nextLiteralID++; }
    void addNewLiteral(Literal *literal);
    Literal *registerLiteral(LOCATION, const Type *type, const LiteralBytes *value);
    virtual LiteralDictionary *clone(Allocator *mem, IRCloner *cloner) const;

    LiteralDictionaryID _id;
    Compiler * _compiler;
    Allocator *_mem;
    String _name;
    LiteralList _literals;
    LiteralList _ownedLiterals;
    std::map<const Type *,LiteralList *> _literalsByType;
    LiteralID _nextLiteralID;
    LiteralDictionary * _linkedDictionary;

    SUBCLASS_KINDSERVICE_DECL(Extensible, LiteralDictionary);
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(LITERALDICTIONARY_INCL)

