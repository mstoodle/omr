/********************************************************************************
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

#include "Builder.hpp"
#include "Compilation.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "Operation.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextWriter.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {

TextWriter::TextWriter(Compilation * comp, std::ostream & os, std::string perIndent)
    : Visitor(comp->compiler())
    , _os(os)
    , _perIndent(perIndent)
    , _indent(0) {

    os << std::showbase // show the 0x prefix
       << std::internal
       << std::setfill('0'); // fill pointers with 0s
}

TextWriter &
operator<<(TextWriter &w, const Builder *b) {
    w << "B" << b->id();
    return w;
}

#ifdef CASES_IN_CORE
TextWriter &
operator<<(TextWriter &w, const Case *c) {
    w << c->value() << " : " << c->builder();
    if (c->fallsThrough())
    w << " fallthrough ";
    return w;
}
#endif

TextWriter &
operator<<(TextWriter &w, const Literal *lv) {
    w << "[ l" << lv->id() << "_" << lv->type() << " ";
    lv->type()->printLiteral(w, lv);
    w << " ]";
    return w;
}

TextWriter &
operator<< (TextWriter &w, const LiteralDictionary *ld) {
    return w << "L" << ld->id();
}

TextWriter &
operator<< (TextWriter &w, const Operation *op) {
    return w << "o" << op->id();
}

TextWriter &
operator<< (TextWriter &w, const Symbol *s) {
    w << "[ s" << s->id() << "_" << s->type() << " \"" << s->name() << "\" ]";
    return w;
}

TextWriter &
operator<< (TextWriter &w, const SymbolDictionary *sd) {
    return w << "S" << sd->id();
}

TextWriter &
operator<<(TextWriter &w, const Type *t) {
    return w << "t" << t->id();
}

TextWriter &
operator<<(TextWriter &w, const TypeDictionary *dict) {
    return w << "T" << dict->id();
}

TextWriter &
operator<<(TextWriter &w, const Value *v) {
    return w << "v" << v->id() << "_" << v->type();
}

void
TextWriter::visitPreCompilation(Compilation * comp) {
    TextWriter &w = *this;
    w << "[ Compilation ";
    w.indentIn(); // won't take effect until next endl!
    comp->write(w);
}

void
TextWriter::visitPostCompilation(Compilation * comp) {
    TextWriter &w = *this;

    w.indentOut();
    w.indent() << "]" << w.endl();
}

void
TextWriter::visitBuilderPreOps(Builder * b) {
    TextWriter &w = *this;
    b->writePrefix(w);
}

void
TextWriter::visitBuilderPostOps(Builder * b) {
    TextWriter &w = *this;
    b->writeSuffix(w);
}

void
TextWriter::printTypePrefix(const Type * type, bool indent) {
    TextWriter &w = *this;
    if (indent)
        w.indent();
    //w << "[ " << ((void*)type) << " type " << type << " " << type->size() << " " << type->name() << " ";
    w << "[ type " << type << " " << type->size() << " " << type->name() << " ";
}

void
TextWriter::writeType(const Type *type, bool indent) {
    TextWriter &w = *this;
    printTypePrefix(type, indent);

#if 0
    // keep here for handy reference until fully migrated
    if (type->isPointer()) {
        PointerType *pType = static_cast<PointerType *>(type);
        w << "pointerType t" << pType->BaseType()->id() << " ]" << w.endl();
    }
    else if (type->isStruct()) {
        StructType *sType = static_cast<StructType *>(type);
        w << "structType";
        for (auto fIt = sType->FieldsBegin(); fIt != sType->FieldsEnd(); fIt++) {
            FieldType *fType = fIt->second;
            w << " t" << fType->id() << "@" << fType->offset();
        }
        w << " ]" << w.endl();
    }
    else if (type->isUnion()) {
        UnionType *uType = static_cast<UnionType *>(type);
        w << "unionType ";
        for (auto fIt = uType->FieldsBegin(); fIt != uType->FieldsEnd(); fIt++) {
            FieldType *fType = fIt->second;
            w << " t" << fType->id();
        }
        w << " ]" << w.endl();
    }
    else if (type->isField()) {
        const FieldType *fType = static_cast<FieldType *>(type);
        w << "fieldType struct t" << fType->owningStruct()->id() << " field t" << fType->type()->id() << " " << fType->name() << " offset " << fType->offset() << " ]" << w.endl();
    }
    else if (type->isFunction()) {
        FunctionType *fType = static_cast<FunctionType *>(type);
        w << "functionType t" << fType->returnType()->id() << " " << fType->numParms();
        for (int32_t p=0;p < fType->numParms();p++)
            w << " t" << fType->parmType(p)->id();
        w << " ]" << w.endl();
    }
    else
#endif
         {
        w << "primitiveType";
        const Type *layout = type->layout();
        if (layout)
            w << " layout " << layout;
        w << "]" << w.endl();
    }
}

void
TextWriter::writeOperation(Operation * op) {
    TextWriter &w = *this;
    op->writeFull(w);

    #if 0
    // leave this here for handy reference until fully migrated

    if (op->isDynamic()) {
        DynamicOperation *dOp = static_cast<DynamicOperation *>(op);
        dOp->print(this);
        return;
    }

    std::string name = actionName(op->action()) + std::string(" ");
    switch (op->action()) {
        case aNone :
            break;

        case aConstInt8 :
            w << op->result() << " = " << name << (int32_t) op->literal()->getInt8() << w.endl();
            break;

        case aConstInt16 :
            w << op->result() << " = " << name << (int32_t) op->literal()->getInt16() << w.endl();
            break;

        case aConstInt32 :
            w << op->result() << " = i" << name << op->literal()->getInt32() << w.endl();
            break;

        case aConstInt64 :
            w << op->result() << " = " << name << op->literal()->getInt64() << w.endl();
            break;

        case aConstFloat :
            w << op->result() << " = " << name << op->literal()->getFloat() << w.endl();
            break;

        case aConstDouble :
            w << op->result() << " = " << name << op->literal()->getDouble() << w.endl();
            break;

        case aConstAddress :
            w << op->result() << " = " << name << op->literal()->getAddress() << w.endl();
            break;

        case aCoercePointer :
            w << op->result() << " = " << name << op->type() << " " << op->operand() << w.endl();
            break;

        case aAdd :
            w << op->result() << " = " << name << op->operand(0) << " " << op->operand(1) << w.endl();
            break;

        case aSub :
            w << op->result() << " = " << name << op->operand(0) << " " << op->operand(1) << w.endl();
            break;

        case aMul :
            w << op->result() << " = " << name << op->operand(0) << " " << op->operand(1) << w.endl();
            break;

        case aLoad :
            w << op->result() << " = " << name << op->symbol() << w.endl();
            break;

        case aLoadAt :
            w << op->result() << " = " << name << op->type() << " " << op->operand() << w.endl();
            break;

        case aLoadField :
            {
            LoadField *lfOp = static_cast<LoadField *>(op);
            const FieldType *fieldType = lfOp->getFieldType();
            const StructType *structType = fieldType->owningStruct();
            w << lfOp->result() << " = " << name << fieldType << " ( " << structType->name() << " . " << fieldType->name() << " ) " << lfOp->operand() << w.endl();
            }
            break;

        case aLoadIndirect :
            {
            LoadIndirect *liOp = static_cast<LoadIndirect *>(op);
            const FieldType *fieldType = liOp->getFieldType();
            const StructType *structType = fieldType->owningStruct();
            w << liOp->result() << " = " << name << fieldType << " ( " << structType->name() << " -> " << fieldType->name() << " ) " << liOp->operand() << w.endl();
            }
            break;

        case aStore :
            w << name << op->symbol() << " " << op->operand() << w.endl();
            break;

        case aStoreField :
            {
            StoreField *sfOp = static_cast<StoreField *>(op);
            const FieldType *fieldType = sfOp->getFieldType();
            const StructType *structType = fieldType->owningStruct();
            w << name << fieldType << " ( " << structType->name() << " . " << fieldType->name() << " ) " << sfOp->operand(0) << " " << sfOp->operand(1) << w.endl();
            }
            break;

        case aStoreIndirect :
            {
            StoreIndirect *siOp = static_cast<StoreIndirect *>(op);
            const FieldType *fieldType = siOp->getFieldType();
            const StructType *structType = fieldType->owningStruct();
            w << name << fieldType << " ( " << structType->name() << " -> " << fieldType->name() << " ) " << siOp->operand(0) << " " << siOp->operand(1) << w.endl();
            }
            break;

        case aStoreAt :
            w << name << op->operand(0) << " " << op->operand(1) << w.endl();
            break;

        case aIndexAt :
            w << op->result() << " = " << name << op->type() << " " << op->operand(0) << " " << op->operand(1) << w.endl();
            break;

        case aCall :
            {
            Call *callOp = static_cast<Call *>(op);
            if (callOp->result())
                w << callOp->result() << " = ";
            w << name << callOp->function() << " " << callOp->numOperands();
            for (int32_t a=0;a < callOp->numArguments(); a++)
                w << " " << callOp->argument(a);
            w << w.endl();
            }
            break;

        case aAppendBuilder :
            {
            Builder * b = op->builder();
            if (b->numOperations() == 0)
                w << name << b << " (Label)" << w.endl();
            else {
                w << name << b << w.endl();
                if (_visitAppendedBuilders) {
                    w.indentIn();
                    start(b);
                    w.indentOut();
                }
            }
            }
            break;

        case aGoto :
            {
            Builder * b = op->builder();
            if (b->numOperations() == 0)
                w << name << b << " (Label)" << w.endl();
            else
                w << name << b << w.endl();
            }
            break;

        case aReturn :
            w << name;
            if (op->numOperands() > 0) {
                for (ValueIterator vIt = op->OperandsBegin(); vIt != op->OperandsEnd(); vIt++) {
                    Value * v = *vIt;
                    w << " " << v;
                }
            }
            else {
                w << " nil";
            }
            w << w.endl();
            break;

        case aIfCmpGreaterThan :
            w << name << op->operand(0) << " " << op->operand(1);
            w << " then " << op->builder() << w.endl();;
            break;

        case aIfCmpLessThan :
            w << name << op->operand(0) << " " << op->operand(1);
            w << " then " << op->builder() << w.endl();
            break;

        case aIfCmpGreaterOrEqual :
            w << name << op->operand(0) << " " << op->operand(1);
            w << " then " << op->builder() << w.endl();;
            break;

        case aIfCmpLessOrEqual :
            w << name << op->operand(0) << " " << op->operand(1);
            w << " then " << op->builder() << w.endl();
            break;

        case aIfThenElse :
            w << name << op->operand() << " then " << op->builder() << " else ";
            w << " else ";
            if (op->numBuilders() == 2)
                w << op->builder(1);
            else
                w << "nil";
            w << w.endl();
            break;

        case aSwitch :
            w << name << op->operand(0);
            for (CaseIterator cIt = op->CasesBegin(); cIt != op->CasesEnd(); cIt++) {
                Case * c = *cIt;
                w << " " << c;
            }
            w << " else " << op->builder(0) << w.endl();
            break;

        case aForLoop :
            if ((bool)op->literal()->getInt8())
                w << "ForLoopUp ";
            else
                w << "ForLoopDn ";
            w << op->symbol() << " : " << op->operand(0) << " to " << op->operand(1) << " by " << op->operand(2);
            w << " body " << op->builder(0);
            if (op->builder(1) != NULL)
                w << " continue " << op->builder(1);
            if (op->builder(2) != NULL)
                w << " break " << op->builder(2);
            w << w.endl();
            break;

        case aCreateLocalArray :
            w << op->result() << " = " << name << op->literal()->getInt32() << " " << op->type() << w.endl();
            break;

        case aCreateLocalStruct :
            w << op->result() << " = " << name << op->type() << w.endl();
            break;

        default :
            assert(0);
            break;
   }
   #endif
}

void
TextWriter::visitOperation(Operation *op) {
    writeOperation(op);
}

} // namespace JitBuilder
} // namespace OMR

