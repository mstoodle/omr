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
#include "Scope.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC(TextLogger)
TextLogger::TextLogger(Allocator *a, std::ostream & os, String perIndent)
    : Allocatable(a)
    , _os(os)
    , _perIndent(perIndent)
    , _indent(0) {

    os << std::showbase // show the 0x prefix
       << std::internal
       << std::setfill('0'); // fill pointers with 0s
}

TextLogger::TextLogger(std::ostream & os, String perIndent)
    : Allocatable()
    , _os(os)
    , _perIndent(perIndent)
    , _indent(0) {

    os << std::showbase // show the 0x prefix
       << std::internal
       << std::setfill('0'); // fill pointers with 0s
}

TextLogger::~TextLogger() {

}

TextLogger &
operator<<(TextLogger &log, const bool v) {
    log._os << v;
    return log;
}

TextLogger &
operator<<(TextLogger &log, const int8_t v) {
    log._os << v;
    return log;
}

TextLogger &
operator<<(TextLogger &log, const int16_t v) {
    log._os << v;
    return log;
}

TextLogger &operator<<(TextLogger &log, const int32_t v) {
    log._os << v;
    return log;
}

TextLogger &operator<<(TextLogger &log, const int64_t v) {
    log._os << v;
    return log;
}

TextLogger &operator<<(TextLogger &log, const uint32_t v) {
    log._os << v;
    return log;
}

TextLogger &operator<<(TextLogger &log, const uint64_t v) {
    log._os << v;
    return log;
}

#if defined(OSX)
TextLogger &operator<<(TextLogger &log, const size_t v) {
    log._os << v;
    return log;
}
#endif

TextLogger &operator<<(TextLogger &log, const void * v) {
    log._os << std::hex << v << std::dec;
    return log;
}

TextLogger &operator<<(TextLogger &log, const float v) {
    log._os << v;
    return log;
}

TextLogger &operator<<(TextLogger &log, const double v) {
    log._os << v;
    return log;
}

TextLogger &operator<<(TextLogger &log, const String & s) {
    s.log(log);
    return log;
}

TextLogger &operator<<(TextLogger &log, const char *s) {
    log._os << s;
    return log;
}

TextLogger &
operator<<(TextLogger &log, const Builder *b) {
    log << "B" << b->id();
    return log;
}

TextLogger &
operator<<(TextLogger & log, const Literal *lv) {
    log << "[ l" << lv->id() << "_" << lv->type() << " ";
    lv->type()->logLiteral(log, lv);
    log << " ]";
    return log;
}

TextLogger &
operator<< (TextLogger &log, const LiteralDictionary *ld) {
    return log << "L" << ld->id();
}

TextLogger &
operator<< (TextLogger &log, const Operation *op) {
    return log << "o" << op->id();
}

TextLogger &
operator<< (TextLogger &log, const Scope *s) {
    log << "[ scope" << s->id() << " \"" << s->name() << "\" ]";
    return log;
}

TextLogger &
operator<< (TextLogger &log, const Symbol *sym) {
    sym->log(log);
    return log;
}

TextLogger &
operator<< (TextLogger &log, const SymbolDictionary *sd) {
    return log << "S" << sd->id();
}

TextLogger &
operator<<(TextLogger &log, const Type *t) {
    return log << "t" << t->id();
}

TextLogger &
operator<<(TextLogger &log, const TypeDictionary *dict) {
    return log << "T" << dict->id();
}

TextLogger &
operator<<(TextLogger &log, const Value *v) {
    return log << "v" << v->id() << "_" << v->type();
}


TextLogger &
TextLogger::tagLine() {
    _os << "============================================================\n";
    return *this;
}

String
TextLogger::sectionBegin() {
    return "{ ";
}

String
TextLogger::sectionStop() {
    return "} ";
}

TextLogger &
TextLogger::taggedSectionStart (String section, String extra) {
    tagLine();
    sectionStart(section) << extra << endl();
    return *this;
}

TextLogger &
TextLogger::taggedSectionEnd (String section, String extra) {
    sectionEnd(section) << extra << endl();
    tagLine();
    return *this;
}

TextLogger &
TextLogger::sectionStart (String section) {
    indent() << sectionBegin() << section << " ";
    indentIn();
    return *this;
}

TextLogger &
TextLogger::sectionEnd(String section) {
    indentOut();
    indent() << sectionStop() << section << " ";
    return *this;
}

String
TextLogger::irStart() {
    return "[ ";
}

String
TextLogger::irStop() {
    return "]";
}

String
TextLogger::irSpacedStop() {
    return " ]";
}

TextLogger &
TextLogger::irListBegin(String name, size_t numEntries) {
    indent() << irStart() << name << " " << numEntries;
    if (numEntries > 0) {
        (*this) << endl();
        indentIn();
    }
    return *this;
}

TextLogger &
TextLogger::irListEnd(size_t numEntries) {
    if (numEntries > 0) {
        indentOut();
        indent();
    } else {
        *this << " ";
    }
    
    *this << irStop() << endl();
    return *this;
}

TextLogger &
TextLogger::irSectionBegin(String title, String designator, uint64_t id, ExtensibleKind kind, String name) {
    const String &kindName = Extensible::kindService.getName(kind);
    indent() << irStart() << title << " " << designator << id << " " << kindName << " \"" << name << "\"" << endl();
    indentIn();
    return *this;
}

TextLogger &
TextLogger::irSectionEnd() {
    indentOut();
    indent() << irSpacedStop() << endl();
    return *this;
}

TextLogger &
TextLogger::irOneLinerBegin(String title, String designator, uint64_t id) {
    indent() << irStart() << title << " " << designator << id << " ";
    return *this;
}

TextLogger &
TextLogger::irOneLinerEnd() {
    (*this) << " " << irStop() << endl();
    return *this;
}

TextLogger &
TextLogger::irFlagBegin(String flag) {
    indent() << irStart() << flag << " ";
    return *this;
}

TextLogger &
TextLogger::irFlagEnd() {
    *this << irStop() << endl();
    return *this;
}

TextLogger &
TextLogger::irBooleanFlag(String flag, bool on) {
    indent() << irStart() << (on ? "" : "not") << flag << irStop() << endl();
    return *this;
}

String
TextLogger::endl() const {
    return String("\n");
}

TextLogger &
TextLogger::indent() {
    for (int32_t in=0;in < _indent;in++)
        _perIndent.log(*this);
    return *this;
}

void
TextLogger::indentIn() {
    _indent++;
}

void
TextLogger::indentOut() {
    _indent--;
}

#if 0
void
TextLogger::logTypePrefix(const Type * type, bool indent) {
    TextLogger &log = *this;
    if (indent)
        log.indent();
    log << "[ type " << type << " " << type->size() << " " << type->name() << " ";
}

void
TextLogger::logType(const Type *type, bool indent) {
    TextLogger &log = *this;
    //logTypePrefix(type, indent);
    type->logType(this, true);

    // keep here for handy reference until fully migrated
    if (type->isPointer()) {
        PointerType *pType = static_cast<PointerType *>(type);
        log << "pointerType t" << pType->BaseType()->id() << " ]" << log.endl();
    }
    else if (type->isStruct()) {
        StructType *sType = static_cast<StructType *>(type);
        log << "structType";
        for (auto fIt = sType->FieldsBegin(); fIt != sType->FieldsEnd(); fIt++) {
            FieldType *fType = fIt->second;
            log << " t" << fType->id() << "@" << fType->offset();
        }
        log << " ]" << log.endl();
    }
    else if (type->isUnion()) {
        UnionType *uType = static_cast<UnionType *>(type);
        log << "unionType ";
        for (auto fIt = uType->FieldsBegin(); fIt != uType->FieldsEnd(); fIt++) {
            FieldType *fType = fIt->second;
            log << " t" << fType->id();
        }
        log << " ]" << log.endl();
    }
    else if (type->isField()) {
        const FieldType *fType = static_cast<FieldType *>(type);
        log << "fieldType struct t" << fType->owningStruct()->id() << " field t" << fType->type()->id() << " " << fType->name() << " offset " << fType->offset() << " ]" << log.endl();
    }
    else if (type->isFunction()) {
        FunctionType *fType = static_cast<FunctionType *>(type);
        log << "functionType t" << fType->returnType()->id() << " " << fType->numParms();
        for (int32_t p=0;p < fType->numParms();p++)
            log << " t" << fType->parmType(p)->id();
        log << " ]" << log.endl();
    }
    else
    {
        log << "primitiveType";
        const Type *layout = type->layout();
        if (layout)
            log << " layout " << layout;
        log << "]" << log.endl();
    }
}
#endif

void
TextLogger::logOperation(Operation * op) {
    TextLogger &log = *this;
    op->logFull(log);

    #if 0
    // leave this here for handy reference until fully migrated

    if (op->isDynamic()) {
        DynamicOperation *dOp = static_cast<DynamicOperation *>(op);
        dOp->print(this);
        return;
    }

    String name = actionName(op->action()) + String(" ");
    switch (op->action()) {
        case aNone :
            break;

        case aConstInt8 :
            log << op->result() << " = " << name << (int32_t) op->literal()->getInt8() << log.endl();
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

} // namespace JB2
} // namespace OMR
