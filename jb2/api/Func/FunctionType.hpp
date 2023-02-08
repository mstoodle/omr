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

#ifndef FUNCTIONTYPES_INCL
#define FUNCTIONTYPES_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

class FunctionExtension;

class FunctionType : public Type {
    JBALLOC_(FunctionType)

    friend class FunctionExtension;
    friend class TypeDictionary;

public:
    #if 0
    // TODO: Move to separate class
    class TypeBuilder {
        struct FieldInfo {
            String _name;
            const Type * _type;
            size_t _offset;
            FieldInfo(String name, const Type *type, size_t offset)
                : _name(name, _type(type), _offset(offset) {
            }
        };
    public:
        TypeBuilder()
            : _ext(NULL)
            , _name("")
            , _size(0)
            , _helper(NULL) {
        }
        TypeBuilder *setExtension(Extension *ext) { _ext = ext; }
        TypeBuilder *setName(String n) { _name = n; }
        TypeBuilder *setSize(size_t size) { _size = size; }
        TypeBuilder *addField(String name, const Type *fieldType, size_t offset) {
            FieldInfo info(name, fieldType, offset);
            _fields.push_back(info);
        }
        TypeBuilder *setHelper(StructHelperFuntion *helper) { _helper = helper; }

        Extention *extension() const { return _extension; }
        String name() const { return _name; }
        size_t size() const { return _size; }
        StructHelperFunction *helper() const { return _helper; }

        void createFields(LOCATION, StructType *structType) {
            for (auto it = _fields.begin(); it != _fields.end(); it++) {
                FieldInfo info = *it;
                structType->addField(PASSLOC, info._name, info._type, info._offset);
            }
        }

        const StructType *create() {
            return StructType::create(this);
        }

    protected:
        Extension * _ext;
        String _name;
        size_t _size;
        List<FieldInfo> _fields;
        StructHelperFunction _helper;
    };
    #endif
    
    Literal *literal(LOCATION, Compilation *comp, void * functionValue);
    virtual String to_string(bool useHeader=false) const;

    const Type *returnType() const { return _returnType; }
    int32_t numParms() const { return _numParms; }
    const Type *parmType(int p) const { return _parmTypes[p]; }
    const Type **parmTypes() const { return _parmTypes; }

    virtual void logValue(TextLogger &lgr, const void *p) const;

    virtual const Type * replace(TypeReplacer *repl);

    static String typeName(const Type *returnType, int32_t numParms, const Type **parmTypes);

    static const TypeKind getTypeClassKind();

protected:
    DYNAMIC_ALLOC_ONLY(FunctionType, LOCATION, Extension *ext, TypeDictionary *dict, const Type *returnType, int32_t numParms, const Type ** parmTypes);

    const Type *_returnType;
    int32_t _numParms;
    const Type **_parmTypes;

    static TypeKind TYPEKIND;
    static bool kindRegistered;

    FunctionExtension *funcExt();
    FunctionExtension *funcExt() const;
};

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(FUNCTIONTYPES_INCL)
