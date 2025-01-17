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

#ifndef OMRCODEGENERATOR_INCL
#define OMRCODEGENERATOR_INCL

#include "jb2/JBCore.hpp"

namespace TR { class Block; }
namespace TR { class CFG; }
namespace TR { class Compilation; }
namespace TR { class DataType; }
namespace TR { class FrontEnd; }
namespace TR { class IlGeneratorMethodDetails; }
namespace TR { class Node; }
namespace TR { class ResolvedMethodSymbol; }
namespace TR { class SymbolReferenceTable; }
namespace TR { class TreeTop; }


namespace OMR {
namespace JB2 {
namespace omrgen {

// Need to transform the incoming IR
//    Type mappings can be set up by iterating over Types in the TypeDictionary
//    Symbols can be created by iterating over SymbolDictionary
//    Two pass code generation, akin to how JB2 does it:
//         1) figure out how many TR::Blocks are needed by iterating over operations and counting
//              Allocate an array and mapping from Builder to start TR::Block
//         2) generate code for each Builder in the target TR::Block
//              Use uses of Value to determine whether needs anchoring or store to a local
//
// Estimating number of TR::Block needed:
//    Each Operation knows how many TR::Blocks it needs to represent its generated code assuming target Builders use no blocks
//    Each Builder will be conceptually replaced by a list of TR::TreeTop that start with the entry point of the Builder
//    Every Builder must contain a control flow tree that explicitly passes to the appropriate successor
//       (optimization) elimiante the control flow if target will directly follow
//
// For Value handling, as Operations are visited, outgoing control flow edges are counted
//    Where Value is defined, record count of control flow edges in parent Builder
//    For Value uses, if parent Builder is different or if control flow edge count at def isn't same as at use, generate a store
//       Otherwise can anchor at a TR::treetop
//
// For Type mapping:
//    Structs need to be flattened
//    Any user type needs to be replaced by its layout class until only BaseExtension Types are present
//
// Location handling:
//    As Locations are encountered, assign bytecode indices and build mapping of Location to bytecode indices and vice versa
//
//


class OMRIlGen;

class OMRCodeGenerator : public CodeGenerator {
    JBALLOC_(OMRCodeGenerator)

    friend class OMRIlGen;

public:
    DYNAMIC_ALLOC_ONLY(OMRCodeGenerator, Extension *ext);

    int32_t returnCode() const { return _compileReturnCode; }
    OMRIlGen *ilgen() const { return _ilgen; }

    virtual CompilerReturnCode perform(Compilation *comp);

    virtual void setupbody(Compilation *comp) { }
    virtual void createbuilder(Builder *b) { }

    virtual void genbody(Compilation *comp) { }
    virtual Builder * gencode(Operation *op);
    virtual void connectsuccessors(Builder *b) { }

    virtual bool registerBuilder(Builder *b)    { return true; }
    virtual bool registerContext(Context *c)    { return true; }
    virtual bool registerLiteral(Literal *lv)   { return true; }
    virtual bool registerScope(Scope *s)        { return true; }
    virtual bool registerSymbol(Symbol *sym)    { return true; }
    virtual bool registerType(const Type *type) { return true; }
    virtual bool registerValue(Value *value)    { return true; }

protected:
    virtual void visitPreCompilation(Compilation * comp);
    virtual void visitBuilderPreOps(Builder * b);
    virtual void visitBuilderPostOps(Builder * b);
    virtual void visitOperation(Operation * op);
    virtual void visitPostCompilation(Compilation *comp);

    void setIlGen(OMRIlGen *ilgen);
    void registerSymbols(Compilation *comp);
    void registerTypes(Compilation *comp);

    OMRIlGen *_ilgen;
    int32_t _omrCompileReturnCode;
    CompilerReturnCode _compileReturnCode;

    SUBCLASS_KINDSERVICE_DECL(Extensible,OMRCodeGenerator);
};

} // namespace omrgen
} // namespace JB2
} // namespace OMR

#endif // !defined(OMRGENCODEGENERATOR_INCL)
