/*******************************************************************************
 * Copyright (c) 2017, 2023 IBM Corp. and others
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

#include <dlfcn.h>
#include <limits>
#include <stdio.h>
#include "gtest/gtest.h"
#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "jb/JB.hpp"

using namespace OMR::JitBuilder;

int main(int argc, char** argv) {
    void *handle = dlopen("libjb2core.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        return -1;
    }

    // creating a Compiler here means JIT will be initialized and shutdown only once
    //  which means all compiled functions can be logged/tracked
    // otherwise JIT will be initialized and shutdown with each test's Compiler instance
    //  and verbose/logs are overwritten and recreated for each Compiler instance
    //  making it much more difficult to log an individual compiled function
    Compiler c("Global");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(BaseExtension, loadExtension) {
    Compiler c("testBase");
    Base::BaseExtension *ext = c.loadExtension<Base::BaseExtension>(LOC, NULL, "jb2base");
    EXPECT_TRUE(ext != NULL) << "Base extension loaded";
}

TEST(BaseExtension, cannotLoadUnknownExtension) {
    Compiler c("testNotBase");
    Base::BaseExtension *ext = c.loadExtension<Base::BaseExtension>(LOC, NULL, "unknown");
    EXPECT_EQ(ext, nullptr) << "notbase extension correctly could not be loaded";
}

TEST(BaseExtension, checkVersionPass) {
    Compiler c("testBase");
    SemanticVersion v((MajorID)0,0,0);
    Base::BaseExtension *ext = c.loadExtension<Base::BaseExtension>(LOC, &v, "jb2base");
    EXPECT_NE(ext, nullptr) << "Base extension with v(0,0,0) loaded";
}

TEST(BaseExtension, checkVersionFail) {
    Compiler c("testBase");
    SemanticVersion v((MajorID)1,0,0); // random version number that doesn't exist yet
    Base::BaseExtension *ext = c.loadExtension<Base::BaseExtension>(LOC, &v, "jb2base");
    EXPECT_EQ(ext, nullptr) << "Base extension with v(1,0,0) correctly could not be loaded";
}

// Base should really not be dependent on the Func extension, but currently there's no way
// to generate code without Func, so all these tests are written to depend on Func

// BaseFunc is an abstract class used by subclasses to create test functions for Base
class BaseFunc : public Func::Function {
public:
    BaseFunc(LOCATION, Compiler *c, String name, String line, String file)
        : Func::Function(PASSLOC, c)
        , _cx(c->lookupExtension<CoreExtension>())
        , _bx(c->lookupExtension<Base::BaseExtension>())
        , _fx(c->lookupExtension<Func::FunctionExtension>()) {
        DefineName(name);
        DefineLine(line);
        DefineFile(file);
    }

protected:
    virtual ~BaseFunc() { }

    // implement in subclasses
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) = 0;
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) = 0;

    CoreExtension *_cx;
    Base::BaseExtension *_bx;
    Func::FunctionExtension *_fx;
    std::map<String, Func::ParameterSymbol *> _parmSymbols;
};

#define BASE_FUNC(name,line,file,fields,entry,xtor_code,init_code,il_code) \
    class name : public BaseFunc { \
    public: \
        name(LOCATION, Compiler *c) : BaseFunc(PASSLOC, c, #name, line, file) { \
            xtor_code; \
        } \
    protected: \
        virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) { \
            Base::BaseCompilationAddon *bao = comp->addon<Base::BaseCompilationAddon>(); \
            init_code \
            return true; \
	    } \
        virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) { \
            Base::BaseCompilationAddon *bao = comp->addon<Base::BaseCompilationAddon>(); \
            Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder(); \
            il_code \
            return true; \
        } \
        fields; \
    };

#define COMPILER(c,cfg,cx,fx,bx) \
    Config cfg; \
    Compiler c("testBase", &cfg); \
    CoreExtension *cx = c.lookupExtension<CoreExtension>(); \
    Func::FunctionExtension *fx = c.loadExtension<Func::FunctionExtension>(LOC); \
    Base::BaseExtension *bx = c.loadExtension<Base::BaseExtension>(LOC); \
    JB::JBExtension *jx = c.loadExtension<JB::JBExtension>(LOC);

#define LOGGING(c,cfg,wrt,DO_LOGGING) \
    TextLogger logger(std::cout, String("    ")); \
    TextLogger *wrt = (DO_LOGGING) ? &logger : NULL; \
    if (DO_LOGGING) { \
        c.config()->setTraceBuildIL(); \
    }
    

//    TextWriter *writer = c.textWriter(logger); \

//    cfg.setTraceCompilerAllocations() \
//       ->setLogger(&logger); \


#define COMPILE_TO_SUCCEED(c,FuncClass,cx,fx,wrt,body,FuncProto,f) \
    FuncClass func(LOC, &c); \
    const StrategyID codegenStrategy = cx->strategyCodegen; \
    CompilerReturnCode result = fx->compile(LOC, &func, codegenStrategy, wrt); \
    EXPECT_EQ((int)result, (int)c.CompileSuccessful) << "Compiled function ok"; \
    CompiledBody *body = func.compiledBody(codegenStrategy); \
    EXPECT_NE(body, nullptr) << "Compiled function ok"; \
    FuncProto *f = body->nativeEntryPoint<FuncProto>(); \
    EXPECT_NE(f, nullptr)

#define COMPILE_TO_FAIL(c,FuncClass,cx,fx,wrt,expectedFailureCode) \
    FuncClass func(LOC, &c); \
    CompilerReturnCode result = fx->compile(LOC, &func, cx->strategyCodegen, wrt); \
    EXPECT_EQ((int)result, (int)expectedFailureCode) << "Function compilation expected to fail";

#define COMPILE_FUNC(FuncClass, FuncProto, f, DO_LOGGING) \
    COMPILER(c,cfg,cx,fx,bx) \
    LOGGING(c,cfg,wrt,DO_LOGGING) \
    COMPILE_TO_SUCCEED(c,FuncClass,cx,fx,wrt,body,FuncProto,f)

#define COMPILE_FUNC_TO_FAIL(FuncClass, expectedFailureCode, DO_LOGGING) \
    COMPILER(c,cfg,cx,fx,bx) \
    LOGGING(c,cfg,wrt,DO_LOGGING) \
    COMPILE_TO_FAIL(c,FuncClass,cx,fx,wrt,expectedFailureCode)


// Test function that returns a constant value
#define CONSTFUNC(type,seq,v) \
    BASE_FUNC(Const ## type ## Function ## seq, "0", #type ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->type); }, \
        { _fx->Return(LOC, b, _bx->Const ## type(LOC, b, v)); })

#define TESTONECONSTFUNC(type,ctype,seq,v) \
    CONSTFUNC(type, seq, v) \
    TEST(BaseExtension, createConst ## type ## Function ## seq) { \
        typedef ctype (FuncProto)(); \
        COMPILE_FUNC(Const ## type ## Function ## seq, FuncProto, f, false); \
        EXPECT_EQ(f(), v) << "Compiled f() returns " << v; \
    }

#define TESTCONSTFUNC(type,ctype,a,b) \
    TESTONECONSTFUNC(type,ctype,1,a) \
    TESTONECONSTFUNC(type,ctype,2,b) \
    TESTONECONSTFUNC(type,ctype,3,(std::numeric_limits<ctype>::min())) \
    TESTONECONSTFUNC(type,ctype,4,(std::numeric_limits<ctype>::max()))

TESTCONSTFUNC(Int8, int8_t, 3, 0)
TESTCONSTFUNC(Int16, int16_t, 3, 0)
TESTCONSTFUNC(Int32, int32_t, 3, 0)
TESTCONSTFUNC(Int64, int64_t, 3, 0)
TESTCONSTFUNC(Float32, float, 3.0, 0.0)
TESTCONSTFUNC(Float64, double, 3.0, 0.0)

// Test function that returns the value of its single parameter
#define TYPEFUNC(type) \
    BASE_FUNC(type ## Function, "0", #type ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->type); ctx->DefineParameter("val", _bx->type); }, \
        { auto parmSym=ctx->LookupLocal("val"); _fx->Return(LOC, b, _fx->Load(LOC, b, parmSym)); })

#define TESTTYPEFUNC(type,ctype,a,b) \
    TYPEFUNC(type) \
    TEST(BaseExtension, create ## type ## Function) { \
        typedef ctype (FuncProto)(ctype); \
        COMPILE_FUNC(type ## Function, FuncProto, f, false); \
        EXPECT_EQ(f(a), a) << "Compiled f(" << a << ") returns " << a; \
        EXPECT_EQ(f(b), b) << "Compiled f(" << b << ") returns " << b; \
        ctype min=std::numeric_limits<ctype>::min(); \
        EXPECT_EQ(f(min), min) << "Compiled f(" << min << ") returns " << min; \
        ctype max=std::numeric_limits<ctype>::max(); \
        EXPECT_EQ(f(max), max) << "Compiled f(" << max << ") returns " << max; \
    }

TESTTYPEFUNC(Int8, int8_t, 3, 0)
TESTTYPEFUNC(Int16, int16_t, 3, 0)
TESTTYPEFUNC(Int32, int32_t, 3, 0)
TESTTYPEFUNC(Int64, int64_t, 3, 0)
TESTTYPEFUNC(Float32, float, 3.0, 0.0)
TESTTYPEFUNC(Float64, double, 3.0, 0.0)

// Address handled specially
TYPEFUNC(Address)
TEST(BaseExtension, createAddressFunction) {
    typedef void * (FuncProto)(void *);
    COMPILE_FUNC(AddressFunction, FuncProto, f, false);
    void *x = NULL; EXPECT_EQ(f(x), x) << "Compiled f(" << x << ") returns " << x;
    void *y = (void *)&x;EXPECT_EQ(f(y), y) << "Compiled f(" << y << ") returns " << y;
    void *z = (void *)(-1);EXPECT_EQ(f(z), z) << "Compiled f(" << z << ") returns " << z;
}

// Test function that loads parm, stores it into a local variable, then loads and returns the local
#define STORETYPEFUNC(type) \
    BASE_FUNC(Store ## type ## Function, "0", "Store" #type ".cpp", \
        Func::LocalSymbol * _val, b, \
        { }, \
        { ctx->DefineReturnType(_bx->type); ctx->DefineParameter("parm", _bx->type); _val = ctx->DefineLocal("val", _bx->type); }, \
        { auto parm=ctx->LookupLocal("parm"); _fx->Store(LOC, b, _val, _fx->Load(LOC, b, parm)); _fx->Return(LOC, b, _fx->Load(LOC, b, _val)); })

#define TESTSTORETYPEFUNC(type,ctype,a,b) \
    STORETYPEFUNC(type) \
    TEST(BaseExtension, createStore ## type ## Function) { \
        typedef ctype (FuncProto)(ctype); \
        COMPILE_FUNC(Store ## type ## Function, FuncProto, f, false); \
        EXPECT_EQ(f(a), a) << "Compiled f(" << a << ") returns " << a; \
        EXPECT_EQ(f(b), b) << "Compiled f(" << b << ") returns " << b; \
        ctype min=std::numeric_limits<ctype>::min(); \
        EXPECT_EQ(f(min), min) << "Compiled f(" << min << ") returns " << min; \
        ctype max=std::numeric_limits<ctype>::max(); \
        EXPECT_EQ(f(max), max) << "Compiled f(" << max << ") returns " << max; \
    }

TESTSTORETYPEFUNC(Int8, int8_t, 3, 0)
TESTSTORETYPEFUNC(Int16, int16_t, 3, 0)
TESTSTORETYPEFUNC(Int32, int32_t, 3, 0)
TESTSTORETYPEFUNC(Int64, int64_t, 3, 0)
TESTSTORETYPEFUNC(Float32, float, 3.0, 0.0)
TESTSTORETYPEFUNC(Float64, double, 3.0, 0.0)

// Address handled specially
STORETYPEFUNC(Address)
TEST(BaseExtension, createStoreAddressFunction) {
    typedef void * (FuncProto)(void *);
    COMPILE_FUNC(StoreAddressFunction, FuncProto, f, false);
    void *x = NULL; EXPECT_EQ(f(x), x) << "Compiled f(" << x << ") returns " << x;
    void *y = (void *)&x;EXPECT_EQ(f(y), y) << "Compiled f(" << y << ") returns " << y;
    void *z = (void *)(-1);EXPECT_EQ(f(z), z) << "Compiled f(" << z << ") returns " << z;
}

// Test function that returns the value pointed to by its single parameter
#define POINTERTOTYPEFUNC(type) \
    BASE_FUNC(PointerTo ## type ## Function, "0", "PointerTo" #type ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->type); ctx->DefineParameter("ptr", _bx->PointerTo(LOC, comp, _bx->type)); }, \
        { auto parmSym = ctx->LookupLocal("ptr"); _fx->Return(LOC, b, _bx->LoadAt(LOC, b, _fx->Load(LOC, b, parmSym))); })

#define TESTPOINTERTOTYPEFUNC(type,ctype,a,b) \
    POINTERTOTYPEFUNC(type) \
    TEST(BaseExtension, createPointer ## type ## Function) { \
        typedef ctype (FuncProto)(ctype *); \
        COMPILE_FUNC(PointerTo ## type ## Function, FuncProto, f, false); \
        ctype x=a; EXPECT_EQ(f(&x), a) << "Compiled f(&" << a << ") returns " << a; \
        ctype y=b; EXPECT_EQ(f(&y), b) << "Compiled f(&" << b << ") returns " << b; \
        ctype min=std::numeric_limits<ctype>::min(); \
        EXPECT_EQ(f(&min), min) << "Compiled f(&min) returns " << min; \
        ctype max=std::numeric_limits<ctype>::max(); \
        EXPECT_EQ(f(&max), max) << "Compiled f(&max) returns " << max; \
    }

TESTPOINTERTOTYPEFUNC(Int8, int8_t, 3, 0)
TESTPOINTERTOTYPEFUNC(Int16, int16_t, 3, 0)
TESTPOINTERTOTYPEFUNC(Int32, int32_t, 3, 0)
TESTPOINTERTOTYPEFUNC(Int64, int64_t, 3, 0)
TESTPOINTERTOTYPEFUNC(Float32, float, 3.0, 0.0)
TESTPOINTERTOTYPEFUNC(Float64, double, 3.0, 0.0)

// Address handled specially
POINTERTOTYPEFUNC(Address)
TEST(BaseExtension, createPointerAddressFunction) {
    typedef void * (FuncProto)(void **);
    COMPILE_FUNC(PointerToAddressFunction, FuncProto, f, false);
    void *a=NULL;
    void *b=(void *)&a; EXPECT_EQ((intptr_t)(f(&b)), (intptr_t)(&a)) << "Compiled f(&" << b << ") returns " << a;
}

// Test function that stores a parameter value through a second pointer parameter
#define STOREPOINTERTOTYPEFUNC(type) \
    BASE_FUNC(StorePointerTo ## type ## Function, "0", "StorePointerTo" #type ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_cx->NoType); \
          ctx->DefineParameter("ptr", _bx->PointerTo(LOC, comp, _bx->type)); \
          ctx->DefineParameter("val", _bx->type); }, \
        { auto ptrParm = ctx->LookupLocal("ptr"); \
          auto valParm = ctx->LookupLocal("val"); \
          _bx->StoreAt(LOC, b, _fx->Load(LOC, b, ptrParm), _fx->Load(LOC, b, valParm)); \
          _fx->Return(LOC, b); })

#define TESTSTOREPOINTERTOTYPEFUNC(type,ctype,a,b) \
    STOREPOINTERTOTYPEFUNC(type) \
    TEST(BaseExtension, createStorePointer ## type ## Function) { \
        typedef void (FuncProto)(ctype *, ctype); \
        COMPILE_FUNC(StorePointerTo ## type ## Function, FuncProto, f, false); \
        ctype d=0xbb; \
        f(&d, a); EXPECT_EQ(d, a) << "Compiled f(&d," << a << ") stored " << a; \
        f(&d, b); EXPECT_EQ(d, b) << "Compiled f(&d," << b << ") stored " << b; \
        ctype min=std::numeric_limits<ctype>::min(); \
        f(&d, min); EXPECT_EQ(d, min) << "Compiled f(&d,min) stored " << min; \
        ctype max=std::numeric_limits<ctype>::max(); \
        f(&d, max); EXPECT_EQ(d, max) << "Compiled f(&d,max) stored " << max; \
    }

TESTSTOREPOINTERTOTYPEFUNC(Int8, int8_t, 3, 0)
TESTSTOREPOINTERTOTYPEFUNC(Int16, int16_t, 3, 0)
TESTSTOREPOINTERTOTYPEFUNC(Int32, int32_t, 3, 0)
TESTSTOREPOINTERTOTYPEFUNC(Int64, int64_t, 3, 0)
TESTSTOREPOINTERTOTYPEFUNC(Float32, float, 3.0, 0.0)
TESTSTOREPOINTERTOTYPEFUNC(Float64, double, 3.0, 0.0)

// Address handled specially
STOREPOINTERTOTYPEFUNC(Address)
TEST(BaseExtension, createStorePointerAddressFunction) {
    typedef void (FuncProto)(void **, void *);
    COMPILE_FUNC(StorePointerToAddressFunction, FuncProto, f, false);
    void *a=(void *)(-1);
    f(&a, NULL); EXPECT_EQ((intptr_t)a, (intptr_t)NULL) << "Compiled f(&a, NULL) stores NULL to a";
}

// Test function that loads and returns a field's value from a struct pointer passed as parameter
#define ONEFIELDSTRUCTTYPEFUNC(type) \
    BASE_FUNC(OneFieldStruct ## type ## Function, "0", "OneFieldStruct_" #type ".cpp", \
        Func::ParameterSymbol *_parm; const Base::StructType *_structType; const Base::PointerType *_pStructType;, b, \
        { }, \
        { Base::StructTypeBuilder stb(_bx, comp); \
          stb.setName(",Struct") \
             ->addField("field", _bx->type, 0); \
          _structType = stb.create(LOC); \
          _pStructType = _bx->PointerTo(LOC, comp, _structType); \
          _parm = ctx->DefineParameter("parm", _pStructType); \
          ctx->DefineReturnType(_bx->type); }, \
        { Value *base = _fx->Load(LOC, b, _parm); \
          const Base::FieldType *field = _structType->LookupField("field"); \
          Value *fieldVal = _bx->LoadFieldAt(LOC, b, field, base); \
          _fx->Return(LOC, b, fieldVal); })

#define TESTONEFIELDTYPESTRUCT(theType,ctype,a,b) \
    ONEFIELDSTRUCTTYPEFUNC(theType) \
    TEST(BaseExtension, createOneFieldStruct ## theType) { \
        typedef struct { ctype field; } TheStructType; \
        typedef ctype (FuncProto)(TheStructType *); \
        COMPILE_FUNC(OneFieldStruct ## theType ## Function, FuncProto, f, false); \
        TheStructType str; \
        str.field = a; ctype w = f(&str); EXPECT_EQ(w, a); \
        str.field = b; ctype x = f(&str); EXPECT_EQ(x, b); \
        ctype min = str.field = std::numeric_limits<ctype>::min(); \
        ctype y = f(&str); EXPECT_EQ(y, min); \
        ctype max = str.field = std::numeric_limits<ctype>::max(); \
        ctype z = f(&str); EXPECT_EQ(z, max); \
    }

TESTONEFIELDTYPESTRUCT(Int8,int8_t,3,0)
TESTONEFIELDTYPESTRUCT(Int16,int16_t,3,0)
TESTONEFIELDTYPESTRUCT(Int32,int32_t,3,0)
TESTONEFIELDTYPESTRUCT(Int64,int64_t,3,0)
TESTONEFIELDTYPESTRUCT(Float32,float,3.0,0.0)
TESTONEFIELDTYPESTRUCT(Float64,double,3.0,0.0)

ONEFIELDSTRUCTTYPEFUNC(Address)
TEST(BaseExtension, createOneFieldStructAddress) {
    typedef struct { void * field; } TheStructType;
    typedef void * (FuncProto)(TheStructType *);
    COMPILE_FUNC(OneFieldStructAddressFunction, FuncProto, f, false);
    TheStructType str;
    str.field = NULL; void * w = f(&str); EXPECT_EQ((intptr_t)w, (intptr_t)NULL);
    void *ptr = (void *)&str;
    str.field = ptr; void * x = f(&str); EXPECT_EQ((intptr_t)x, (intptr_t)ptr);
}

// Test function that loads and returns the fifth field's value from a struct pointer passed as parameter
#define FIVEFIELDSTRUCTTYPEFUNC(type,ctype) \
    BASE_FUNC(FiveFieldStruct ## type ## Function, "0", "FiveFieldStruct_" #type ".cpp", \
        Func::ParameterSymbol *_parm; const Base::StructType *_structType; const Base::PointerType *_pStructType;, b, \
        { }, \
        { Base::StructTypeBuilder stb(_bx, comp); \
          typedef struct { ctype f1; ctype f2; ctype f3; ctype f4; ctype f5; } TheStructType; \
          stb.setName("Struct") \
             ->addField("f1", _bx->type, 8*offsetof(TheStructType,f1)) \
             ->addField("f2", _bx->type, 8*offsetof(TheStructType,f2)) \
             ->addField("f3", _bx->type, 8*offsetof(TheStructType,f3)) \
             ->addField("f4", _bx->type, 8*offsetof(TheStructType,f4)) \
             ->addField("f5", _bx->type, 8*offsetof(TheStructType,f5)); \
          _structType = stb.create(LOC); \
          _pStructType = _bx->PointerTo(LOC, comp, _structType); \
          _parm = ctx->DefineParameter("parm", _pStructType); \
          ctx->DefineReturnType(_bx->type); }, \
        { Value *base = _fx->Load(LOC, b, _parm); \
          const Base::FieldType *field = _structType->LookupField("f5"); \
          Value *fieldVal = _bx->LoadFieldAt(LOC, b, field, base); \
          _fx->Return(LOC, b, fieldVal); })

#define TESTFIVEFIELDTYPESTRUCT(theType,ctype,a,b) \
    FIVEFIELDSTRUCTTYPEFUNC(theType,ctype) \
    TEST(BaseExtension, createFiveFieldStruct ## theType) { \
        typedef struct { ctype f1; ctype f2; ctype f3; ctype f4; ctype f5; } TheStructType; \
        typedef ctype (FuncProto)(TheStructType *); \
        COMPILE_FUNC(FiveFieldStruct ## theType ## Function, FuncProto, f, false); \
        TheStructType str; \
        str.f5 = a; ctype w = f(&str); EXPECT_EQ(w, a); \
        str.f5 = b; ctype x = f(&str); EXPECT_EQ(x, b); \
        ctype min = str.f5 = std::numeric_limits<ctype>::min(); \
        ctype y = f(&str); EXPECT_EQ(y, min); \
        ctype max = str.f5 = std::numeric_limits<ctype>::max(); \
        ctype z = f(&str); EXPECT_EQ(z, max); \
    }

TESTFIVEFIELDTYPESTRUCT(Int8,int8_t,3,0)
TESTFIVEFIELDTYPESTRUCT(Int16,int16_t,3,0)
TESTFIVEFIELDTYPESTRUCT(Int32,int32_t,3,0)
TESTFIVEFIELDTYPESTRUCT(Int64,int64_t,3,0)
TESTFIVEFIELDTYPESTRUCT(Float32,float,3.0,0.0)
TESTFIVEFIELDTYPESTRUCT(Float64,double,3.0,0.0)

FIVEFIELDSTRUCTTYPEFUNC(Address,void*)
TEST(BaseExtension, createFiveFieldStructAddress) {
    typedef struct { void * f1; void * f2; void * f3; void * f4; void * f5; } TheStructType;
    typedef void * (FuncProto)(TheStructType *);
    COMPILE_FUNC(FiveFieldStructAddressFunction, FuncProto, f, false);
    TheStructType str;
    str.f5 = NULL; void * w = f(&str); EXPECT_EQ((intptr_t)w, (intptr_t)NULL);
    void *ptr = (void *)&str;
    str.f5 = ptr; void * x = f(&str); EXPECT_EQ((intptr_t)x, (intptr_t)ptr);
}

// Test function that stores a parameter to the fifth field's value in a struct pointer also passed as parameter
#define STOREFIVEFIELDSTRUCTTYPEFUNC(type,ctype) \
    BASE_FUNC(StoreFiveFieldStruct ## type ## Function, "0", "StoreFiveFieldStruct_" #type ".cpp", \
        Func::ParameterSymbol *_valParm; Func::ParameterSymbol *_baseParm; const Base::StructType *_structType; const Base::PointerType *_pStructType;, b, \
        { }, \
        { Base::StructTypeBuilder stb(_bx, comp); \
          typedef struct { ctype f1; ctype f2; ctype f3; ctype f4; ctype f5; } TheStructType; \
          stb.setName("Struct") \
             ->addField("f1", _bx->type, 8*offsetof(TheStructType,f1)) \
             ->addField("f2", _bx->type, 8*offsetof(TheStructType,f2)) \
             ->addField("f3", _bx->type, 8*offsetof(TheStructType,f3)) \
             ->addField("f4", _bx->type, 8*offsetof(TheStructType,f4)) \
             ->addField("f5", _bx->type, 8*offsetof(TheStructType,f5)); \
          _structType = stb.create(LOC); \
          _pStructType = _bx->PointerTo(LOC, comp, _structType); \
          _valParm = ctx->DefineParameter("val", _bx->type); \
          _baseParm = ctx->DefineParameter("pStruct", _pStructType); \
          ctx->DefineReturnType(_cx->NoType); }, \
        { Value *base = _fx->Load(LOC, b, _baseParm); \
          const Base::FieldType *field = _structType->LookupField("f5"); \
          Value *val = _fx->Load(LOC, b, _valParm); \
          _bx->StoreFieldAt(LOC, b, field, base, val); \
          _fx->Return(LOC, b); })

#define TESTSTOREFIVEFIELDTYPESTRUCT(theType,ctype,a,b) \
    STOREFIVEFIELDSTRUCTTYPEFUNC(theType,ctype) \
    TEST(BaseExtension, createStoreFiveFieldStruct ## theType) { \
        typedef struct { ctype f1; ctype f2; ctype f3; ctype f4; ctype f5; } TheStructType; \
        typedef void (FuncProto)(ctype, TheStructType *); \
        COMPILE_FUNC(StoreFiveFieldStruct ## theType ## Function, FuncProto, f, false); \
        TheStructType str; \
        f(a, &str); ctype w=str.f5; EXPECT_EQ(w, a); \
        f(b, &str); ctype x=str.f5; EXPECT_EQ(x, b); \
        ctype min = std::numeric_limits<ctype>::min(); \
        f(min, &str); ctype y = str.f5; EXPECT_EQ(y, min); \
        ctype max = str.f5 = std::numeric_limits<ctype>::max(); \
        f(max, &str); ctype z = str.f5; EXPECT_EQ(z, max); \
    }

TESTSTOREFIVEFIELDTYPESTRUCT(Int8,int8_t,3,0)
TESTSTOREFIVEFIELDTYPESTRUCT(Int16,int16_t,3,0)
TESTSTOREFIVEFIELDTYPESTRUCT(Int32,int32_t,3,0)
TESTSTOREFIVEFIELDTYPESTRUCT(Int64,int64_t,3,0)
TESTSTOREFIVEFIELDTYPESTRUCT(Float32,float,3.0,0.0)
TESTSTOREFIVEFIELDTYPESTRUCT(Float64,double,3.0,0.0)

STOREFIVEFIELDSTRUCTTYPEFUNC(Address,void*)
TEST(BaseExtension, createStoreFiveFieldStructAddress) {
    typedef struct { void * f1; void * f2; void * f3; void * f4; void * f5; } TheStructType;
    typedef void (FuncProto)(void *, TheStructType *);
    COMPILE_FUNC(StoreFiveFieldStructAddressFunction, FuncProto, f, false);
    TheStructType str;
    f(NULL, &str); void * w = str.f5; EXPECT_EQ((intptr_t)w, (intptr_t)NULL);
    void *ptr = (void *)&str;
    f(ptr, &str); void * x = str.f5; EXPECT_EQ((intptr_t)x, (intptr_t)ptr);
}

// Test function that loads f2 from a parameter struct, stores it to f2 of a locally allocated struct, then loads f2 again and returns it
#define CREATESTRUCTFUNC(type1,type2,type3,ctype1,ctype2,ctype3) \
    BASE_FUNC(CreateStruct_ ## type1 ## _ ## type2 ## _ ## type3 ## _Function, "0", "CreateStruct_" #type1 "_" #type2 "_" #type3 ".cpp", \
        Func::ParameterSymbol *_parm; const Base::StructType *_structType; const Base::FieldType *_f2Type; const Base::PointerType *_pStructType;, b, \
        { }, \
        { Base::StructTypeBuilder stb(_bx, comp); \
          typedef struct { ctype1 f1; ctype2 f2; ctype3 f3; } cStruct; \
          stb.setName("MyStruct") \
             ->addField("f1", _bx->type1, 8*offsetof(cStruct, f1)) \
             ->addField("f2", _bx->type2, 8*offsetof(cStruct, f2)) \
             ->addField("f3", _bx->type3, 8*offsetof(cStruct, f3)); \
          _structType = stb.create(LOC); \
          _pStructType = _bx->PointerTo(LOC, comp, _structType); \
          _f2Type = _structType->LookupField("f2"); \
          _parm = ctx->DefineParameter("parm", _pStructType); \
          ctx->DefineReturnType(_bx->type2); }, \
        { Value *base = _fx->Load(LOC, b, _parm); \
          Value *f2val_parm = _bx->LoadFieldAt(LOC, b, _f2Type, base); \
          Value *pLocalStruct = _bx->CreateLocalStruct(LOC, b, _pStructType); \
          _bx->StoreFieldAt(LOC, b, _f2Type, pLocalStruct, f2val_parm); \
          Value *f2val_local = _bx->LoadFieldAt(LOC, b, _f2Type, pLocalStruct); \
          _fx->Return(LOC, b, f2val_local); })

#define TESTCREATESTRUCT(type1,type2,type3,ctype1,ctype2,ctype3,a,b) \
    CREATESTRUCTFUNC(type1,type2,type3,ctype1,ctype2,ctype3) \
    TEST(BaseExtension, createStruct_ ## type1 ## _ ## type2 ## _ ## type3) { \
        typedef struct { ctype1 f1; ctype2 f2; ctype3 f3; } TheStructType; \
        typedef ctype2 (FuncProto)(TheStructType *); \
        COMPILE_FUNC(CreateStruct_ ## type1 ## _ ## type2 ## _ ## type3 ## _Function, FuncProto, f, false); \
        TheStructType str; \
        str.f1 = 0; str.f2 = a; str.f3 = 0; \
         ctype1 w1 = str.f1; EXPECT_EQ(w1,0); \
         ctype2 w2 = f(&str); EXPECT_EQ(w2, a); \
         ctype3 w3 = str.f3; EXPECT_EQ(w3,0); \
        str.f1 = 1; str.f2 = b; str.f3 = 1; \
         ctype1 x1 = str.f1; EXPECT_EQ(x1,1); \
         ctype2 x2 = f(&str); EXPECT_EQ(x2, b); \
         ctype3 x3 = str.f3; EXPECT_EQ(x3,1); \
        str.f1 = 2; str.f3 = 2; \
        ctype2 min = str.f2 = std::numeric_limits<ctype2>::min(); \
         ctype1 y1 = str.f1; EXPECT_EQ(y1,2); \
         ctype2 y2 = f(&str); EXPECT_EQ(y2, min); \
         ctype3 y3 = str.f3; EXPECT_EQ(y3,2); \
        str.f1 = -1; str.f3 = -1; \
        ctype2 max = str.f2 = std::numeric_limits<ctype2>::max(); \
         ctype1 z1 = str.f1; EXPECT_EQ(z1,-1); \
         ctype2 z2 = f(&str); EXPECT_EQ(z2, max); \
         ctype1 z3 = str.f3; EXPECT_EQ(z3,-1); \
    }

TESTCREATESTRUCT(Int16,Int8,Int8,int16_t,int8_t,int8_t,3,0)
TESTCREATESTRUCT(Int32,Int16,Int16,int32_t,int16_t,int16_t,3,0)
TESTCREATESTRUCT(Int64,Int32,Int32,int64_t,int32_t,int32_t,3,0)
TESTCREATESTRUCT(Int64,Int64,Int64,int64_t,int64_t,int64_t,3,0)
TESTCREATESTRUCT(Int32,Float32,Int64,int32_t,float,int64_t,3.0,0.0)
TESTCREATESTRUCT(Int64,Float64,Int32,int64_t,double,int32_t,3.0,0.0)

CREATESTRUCTFUNC(Int32,Address,Int32,int32_t,void *,int32_t)
TEST(BaseExtension, createStruct_Int32_Address_Int32) {
    typedef struct { int32_t f1; void * f2; int32_t f3; } TheStructType;
    typedef void * (FuncProto)(TheStructType *);
    COMPILE_FUNC(CreateStruct_Int32_Address_Int32_Function, FuncProto, f, false);
    TheStructType str;
    str.f1 = 0; str.f3 = 0;
    str.f2 = NULL;
     int32_t w1 = str.f1; EXPECT_EQ(w1,0);
     void * w2 = f(&str); EXPECT_EQ((uintptr_t)w2, (uintptr_t)NULL);
     int32_t w3 = str.f3; EXPECT_EQ(w3,0);
    str.f1 = 1; str.f3 = 1;
    str.f2 = (void *)&str;
     int32_t x1 = str.f1; EXPECT_EQ(x1,1);
     void * x2 = f(&str); EXPECT_EQ((uintptr_t)x2, (uintptr_t)&str);
     int32_t x3 = str.f3; EXPECT_EQ(x3,1);
}

typedef struct MyRecursiveStruct {
    int x;
    struct MyRecursiveStruct *next;
} MyRecursiveStruct;

void
MyRecursiveStructHelper(const Base::StructType *sType, Base::StructTypeBuilder *builder) {
    Base::BaseExtension *ext = builder->extension();
    builder->addField("x", ext->Int32, 8*offsetof(MyRecursiveStruct, x))
           ->addField("next", ext->PointerTo(LOC, builder->comp(), sType), 8*offsetof(MyRecursiveStruct, next));
}

BASE_FUNC(CreateRecursiveStructFunction, "0", "CreateRecursiveStruct.cpp", \
    Func::ParameterSymbol *_parm; \
    const Base::StructType *_structType; \
    const Base::FieldType *_xType; \
    const Base::FieldType *_nextType; \
    const Base::PointerType *_pStructType; \
    , b, \
    { }, \
    { Base::StructTypeBuilder stb(_bx, comp); \
      stb.setName("MyRecursiveStruct") \
         ->setHelper(&MyRecursiveStructHelper); \
      _structType = stb.create(LOC); \
      _pStructType = _bx->PointerTo(LOC, comp, _structType); \
      _parm = ctx->DefineParameter("parm", _pStructType); \
      _nextType = _structType->LookupField("next"); \
      _xType = _structType->LookupField("x"); \
      ctx->DefineReturnType(_bx->Int32); }, \
    { Value *base = _fx->Load(LOC, b, _parm); \
      Value *nextval = _bx->LoadFieldAt(LOC, b, _nextType, base); \
      Value *nextnextval = _bx->LoadFieldAt(LOC, b, _nextType, nextval); \
      Value *nextnextxval = _bx->LoadFieldAt(LOC, b, _xType, nextnextval); \
      _fx->Return(LOC, b, nextnextxval); })

TEST(BaseExtension, createRecursiveStructFunction) {
    typedef int32_t (FuncProto)(MyRecursiveStruct *);
    COMPILE_FUNC(CreateRecursiveStructFunction, FuncProto, f, false);
    int32_t value = 3;
    MyRecursiveStruct third = {value, NULL};
    MyRecursiveStruct second = {-2, &third};
    MyRecursiveStruct first = {-1, &second};
    EXPECT_EQ(f(&first), value);
}

// Test function that returns an indexed value from an array parameter
#define ARRAYTYPEFUNC(type) \
    BASE_FUNC(type ## ArrayFunction, "0", #type ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->type); \
          ctx->DefineParameter("array", _bx->PointerTo(LOC, comp, _bx->type)); \
          ctx->DefineParameter("index", _bx->Int32); }, \
        { auto arraySym=ctx->LookupLocal("array"); \
          Value * array = _fx->Load(LOC, b, arraySym); \
          auto indexSym=ctx->LookupLocal("index"); \
          Value * index = _fx->Load(LOC, b, indexSym); \
          Value * pElement = _bx->IndexAt(LOC, b, array, index); \
          Value * element = _bx->LoadAt(LOC, b, pElement); \
          _fx->Return(LOC, b, element); })

#define TESTARRAYTYPEFUNC(type,ctype,ai,a,bi,b,mini,maxi) \
    ARRAYTYPEFUNC(type) \
    TEST(BaseExtension, create ## type ## ArrayFunction) { \
        typedef ctype (FuncProto)(ctype *, int32_t); \
        COMPILE_FUNC(type ## ArrayFunction, FuncProto, f, false); \
        ctype array[32]; \
        int32_t i=0; \
        for (i=0;i < 32;i++) array[i] = -1; \
        i=ai; array[i] = a; EXPECT_EQ(f(array,i), a) << "Compiled f(array," << i << ") returns " << a; \
        i=bi; array[i] = b; EXPECT_EQ(f(array,i), b) << "Compiled f(array," << i << ") returns " << b; \
        ctype min=std::numeric_limits<ctype>::min(); \
        i=mini; array[i] = min; EXPECT_EQ(f(array,i), min) << "Compiled f(array," << i << ") returns " << min; \
        ctype max=std::numeric_limits<ctype>::max(); \
        i=maxi; array[i] = max; EXPECT_EQ(f(array,i), max) << "Compiled f(array," << i << ") returns " << max; \
    }

TESTARRAYTYPEFUNC(Int8, int8_t, 1, 3, 7, 0, 13, 19)
TESTARRAYTYPEFUNC(Int16, int16_t, 2, 3, 8, 0, 14, 20)
TESTARRAYTYPEFUNC(Int32, int32_t, 3, 3, 9, 0, 15, 21)
TESTARRAYTYPEFUNC(Int64, int64_t, 4, 3, 10, 0, 16, 22)
TESTARRAYTYPEFUNC(Float32, float, 5, 3.0, 11, 0.0, 17, 23)
TESTARRAYTYPEFUNC(Float64, double, 6, 3.0, 12, 0.0, 18, 24)

// Address handled specially
ARRAYTYPEFUNC(Address)
TEST(BaseExtension, createAddressArrayFunction) {
    typedef void * (FuncProto)(void **, int32_t);
    COMPILE_FUNC(AddressArrayFunction, FuncProto, f, false);
    void * array[32];
    int32_t i=0;
    for (i=0;i < 32;i++) array[i] = (void *)(uintptr_t)-1;
    i=7; array[i] = NULL; EXPECT_EQ((uintptr_t)f(array,i), (uintptr_t)NULL) << "Compiled f(array," << i << ") returns " << NULL;
    i=9; array[i] = array; EXPECT_EQ((uintptr_t)f(array,i), (uintptr_t)array) << "Compiled f(array," << i << ") returns " << array;
    i=11; array[i] = array+20; EXPECT_EQ((uintptr_t)f(array,i), (uintptr_t)(array+20)) << "Compiled f(array," << i << ") returns " << (array+20);
    i=13; array[i] = array+38; EXPECT_EQ((uintptr_t)f(array,i), (uintptr_t)(array+38)) << "Compiled f(array," << i << ") returns " << (array+38);
}

// Test function that returns the sum of two values of a type
#define ADDTWOTYPEFUNC(leftType,rightType,suffix) \
    BASE_FUNC(leftType ## _ ## rightType ## _AddFunction ## suffix, "0", #leftType "_" #rightType ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->leftType); \
          ctx->DefineParameter("left", _bx->leftType); \
          ctx->DefineParameter("right", _bx->rightType); }, \
        { auto leftSym=ctx->LookupLocal("left"); \
          Value * left = _fx->Load(LOC, b, leftSym); \
          auto rightSym=ctx->LookupLocal("right"); \
          Value * right = _fx->Load(LOC, b, rightSym); \
          Value * sum = _bx->Add(LOC, b, left, right); \
          _fx->Return(LOC, b, sum); })

#define ADDTYPEFUNC(type) ADDTWOTYPEFUNC(type,type,)

#define TESTADDTYPEFUNC(type,ctype,a1,b1,a2,b2) \
    ADDTYPEFUNC(type) \
    TEST(BaseExtension, create ## type ## AddFunction) { \
        typedef ctype (FuncProto)(ctype, ctype); \
        COMPILE_FUNC(type ## _ ## type ## _AddFunction, FuncProto, f, false); \
        ctype x1=a1; ctype x2=a2; ctype y1=b1; ctype y2=b2; \
        EXPECT_EQ(f(x1,y1), (ctype)(x1+y1)) << "Compiled f(x1,y1) returns " << (ctype)(x1+y1); \
        EXPECT_EQ(f(x2,y2), (ctype)(x2+y2)) << "Compiled f(x2,y2) returns " << (ctype)(x2+y2); \
        ctype min=std::numeric_limits<ctype>::min(); \
        EXPECT_EQ(f(min,x1), (ctype)(min+x1)) << "Compiled f(min,x1) returns " << (ctype)(min+x1); \
        EXPECT_EQ(f(min,y1), (ctype)(min+y1)) << "Compiled f(min,y1) returns " << (ctype)(min+y1); \
        EXPECT_EQ(f(min,x2), (ctype)(min+x2)) << "Compiled f(min,x2) returns " << (ctype)(min+x2); \
        EXPECT_EQ(f(min,y2), (ctype)(min+y2)) << "Compiled f(min,y2) returns " << (ctype)(min+y2); \
        ctype max=std::numeric_limits<ctype>::max(); \
        EXPECT_EQ(f(max,x1), (ctype)(max+x1)) << "Compiled f(max,x1) returns " << (ctype)(max+x1); \
        EXPECT_EQ(f(max,y1), (ctype)(max+y1)) << "Compiled f(max,y1) returns " << (ctype)(max+y1); \
        EXPECT_EQ(f(max,x2), (ctype)(max+x2)) << "Compiled f(max,x2) returns " << (ctype)(max+x2); \
        EXPECT_EQ(f(max,y2), (ctype)(max+y2)) << "Compiled f(max,y2) returns " << (ctype)(max+y2); \
    }

TESTADDTYPEFUNC(Int8, int8_t, 0, 1, 1, -1)
TESTADDTYPEFUNC(Int16, int16_t, 0, 1, 1, -1)
TESTADDTYPEFUNC(Int32, int32_t, 0, 1, 1, -1)
TESTADDTYPEFUNC(Int64, int64_t, 0, 1, 1, -1)
TESTADDTYPEFUNC(Float32, float, 0.0, 1.0, 1.0, -1.0)
TESTADDTYPEFUNC(Float64, double, 0.0, 1.0, 1.0, -1.0)

// Address handled specially
ADDTWOTYPEFUNC(Address,Word,)
TEST(BaseExtension, createAddressAddFunction) {
    typedef void * (FuncProto)(void *, size_t);
    COMPILE_FUNC(Address_Word_AddFunction, FuncProto, f, false);
    void *p[2];
    EXPECT_EQ((uintptr_t)f(p,0), (uintptr_t)(p+0)) << "Compiled f(p,0) returns " << (p+0);
    EXPECT_EQ((uintptr_t)f(p,1), (uintptr_t)((uint8_t *)(p)+1)) << "Compiled f(p,1) returns " << (uint8_t *)(p) + 1;
    EXPECT_EQ((uintptr_t)f(p,sizeof(void*)), (uintptr_t)(p+1)) << "Compiled f(p,sizeof(void*)) returns " << p + 1;
}

#define TESTADDTYPESINVALID(leftType,rightType) \
    ADDTWOTYPEFUNC(leftType,rightType,Validity) \
    TEST(BaseExtension, testAddTypesInvalid_ ## leftType ## rightType ) { \
        COMPILE_FUNC_TO_FAIL(leftType ## _ ## rightType ## _AddFunctionValidity , c.lookupExtension<Base::BaseExtension>()->CompileFail_BadInputTypes_Add, false); \
    }

#define TESTBADADDTYPES(leftType,bad1,bad2,bad3,bad4,bad5) \
    TESTADDTYPESINVALID(leftType,bad1); \
    TESTADDTYPESINVALID(leftType,bad2); \
    TESTADDTYPESINVALID(leftType,bad3); \
    TESTADDTYPESINVALID(leftType,bad4); \
    TESTADDTYPESINVALID(leftType,bad5);

TESTBADADDTYPES(Int8,Int16,Int32,Int64,Float32,Float64)
TESTBADADDTYPES(Int16,Int8,Int32,Int64,Float32,Float64)
TESTBADADDTYPES(Int32,Int8,Int16,Int64,Float32,Float64)
TESTBADADDTYPES(Int64,Int8,Int16,Int32,Float32,Float64)
TESTADDTYPESINVALID(Address,Int8);
TESTADDTYPESINVALID(Int8,Address);
TESTADDTYPESINVALID(Address,Int16);
TESTADDTYPESINVALID(Int16,Address);
#if PLATFORM_32BIT
    TESTADDTYPESINVALID(Address,Int64);
    TESTADDTYPESINVALID(Int64,Address);
#else
    TESTADDTYPESINVALID(Address,Int32);
    TESTADDTYPESINVALID(Int32,Address);
#endif
TESTADDTYPESINVALID(Address,Float32);
TESTADDTYPESINVALID(Float32,Address);
TESTADDTYPESINVALID(Address,Float64);
TESTADDTYPESINVALID(Float64,Address);
TESTBADADDTYPES(Float32,Int8,Int16,Int32,Int64,Float64)
TESTBADADDTYPES(Float64,Int8,Int16,Int32,Int64,Float32)

// Test function that returns the product of two values of a type
#define MULTWOTYPEFUNC(leftType,rightType,suffix) \
    BASE_FUNC(leftType ## _ ## rightType ## _MulFunction ## suffix, "0", #leftType "_" #rightType ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->leftType); \
          ctx->DefineParameter("left", _bx->leftType); \
          ctx->DefineParameter("right", _bx->rightType); }, \
        { auto leftSym=ctx->LookupLocal("left"); \
          Value * left = _fx->Load(LOC, b, leftSym); \
          auto rightSym=ctx->LookupLocal("right"); \
          Value * right = _fx->Load(LOC, b, rightSym); \
          Value * prod = _bx->Mul(LOC, b, left, right); \
          _fx->Return(LOC, b, prod); })

#define MULTYPEFUNC(type) MULTWOTYPEFUNC(type,type,)

#define TESTMULTYPEFUNC(type,ctype,a1,b1,a2,b2) \
    MULTYPEFUNC(type) \
    TEST(BaseExtension, create ## type ## _ ## type ## _MulFunction) { \
        typedef ctype (FuncProto)(ctype, ctype); \
        COMPILE_FUNC(type ## _ ## type ## _MulFunction, FuncProto, f, false); \
        ctype x1=a1; ctype x2=a2; ctype y1=b1; ctype y2=b2; \
        EXPECT_EQ(f(x1,y1), (ctype)(x1*y1)) << "Compiled f(x1,y1) returns " << (ctype)(x1*y1); \
        EXPECT_EQ(f(x2,y2), (ctype)(x2*y2)) << "Compiled f(x2,y2) returns " << (ctype)(x2*y2); \
        ctype min=std::numeric_limits<ctype>::min(); \
        EXPECT_EQ(f(min,x1), (ctype)(min*x1)) << "Compiled f(min,x1) returns " << (ctype)(min*x1); \
        EXPECT_EQ(f(min,y1), (ctype)(min*y1)) << "Compiled f(min,y1) returns " << (ctype)(min*y1); \
        EXPECT_EQ(f(min,x2), (ctype)(min*x2)) << "Compiled f(min,x2) returns " << (ctype)(min*x2); \
        EXPECT_EQ(f(min,y2), (ctype)(min*y2)) << "Compiled f(min,y2) returns " << (ctype)(min*y2); \
        ctype max=std::numeric_limits<ctype>::max(); \
        EXPECT_EQ(f(max,x1), (ctype)(max*x1)) << "Compiled f(max,x1) returns " << (ctype)(max*x1); \
        EXPECT_EQ(f(max,y1), (ctype)(max*y1)) << "Compiled f(max,y1) returns " << (ctype)(max*y1); \
        EXPECT_EQ(f(max,x2), (ctype)(max*x2)) << "Compiled f(max,x2) returns " << (ctype)(max*x2); \
        EXPECT_EQ(f(max,y2), (ctype)(max*y2)) << "Compiled f(max,y2) returns " << (ctype)(max*y2); \
    }

TESTMULTYPEFUNC(Int8, int8_t, 0, 1, 2, -1)
TESTMULTYPEFUNC(Int16, int16_t, 0, 1, 2, -1)
TESTMULTYPEFUNC(Int32, int32_t, 0, 1, 2, -1)
TESTMULTYPEFUNC(Int64, int64_t, 0, 1, 2, -1)
TESTMULTYPEFUNC(Float32, float, 0.0, 2.0, 1.0, -1.0)
TESTMULTYPEFUNC(Float64, double, 0.0, 2.0, 1.0, -1.0)

#define TESTMULTYPESINVALID(leftType,rightType) \
    MULTWOTYPEFUNC(leftType,rightType,Validity) \
    TEST(BaseExtension, testMulTypesInvalid_ ## leftType ## rightType ) { \
        COMPILE_FUNC_TO_FAIL(leftType ## _ ## rightType ## _MulFunctionValidity , c.lookupExtension<Base::BaseExtension>()->CompileFail_BadInputTypes_Mul, false); \
    }

#define TESTBADMULTYPES(leftType,bad1,bad2,bad3,bad4,bad5,bad6) \
    TESTMULTYPESINVALID(leftType,bad1); \
    TESTMULTYPESINVALID(leftType,bad2); \
    TESTMULTYPESINVALID(leftType,bad3); \
    TESTMULTYPESINVALID(leftType,bad4); \
    TESTMULTYPESINVALID(leftType,bad5); \
    TESTMULTYPESINVALID(leftType,bad6);

TESTBADMULTYPES(Int8,Int16,Int32,Int64,Float32,Float64,Address)
TESTBADMULTYPES(Int16,Int8,Int32,Int64,Float32,Float64,Address)
TESTBADMULTYPES(Int32,Int8,Int16,Int64,Float32,Float64,Address)
TESTBADMULTYPES(Int64,Int8,Int16,Int32,Float32,Float64,Address)
TESTBADMULTYPES(Float32,Int8,Int16,Int32,Int64,Float64,Address)
TESTBADMULTYPES(Float64,Int8,Int16,Int32,Int64,Float32,Address)
TESTBADMULTYPES(Address,Int8,Int16,Int32,Int64,Float32,Float64)
TESTMULTYPESINVALID(Address,Address);

// Test function that returns the difference of two values of a type
#define SUBTYPEFUNC(returnType,leftType,rightType,suffix) \
    BASE_FUNC(returnType ## _ ## leftType ## _ ## rightType ## _SubFunction ## suffix, "0", #returnType "_" #leftType "_" #rightType ".cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->returnType); \
          ctx->DefineParameter("left", _bx->leftType); \
          ctx->DefineParameter("right", _bx->rightType); }, \
        { auto leftSym=ctx->LookupLocal("left"); \
          Value * left = _fx->Load(LOC, b, leftSym); \
          auto rightSym=ctx->LookupLocal("right"); \
          Value * right = _fx->Load(LOC, b, rightSym); \
          Value * sum = _bx->Sub(LOC, b, left, right); \
          _fx->Return(LOC, b, sum); })

#define TESTSUBTYPEFUNC(type,ctype,a1,b1,a2,b2) \
    SUBTYPEFUNC(type,type,type,) \
    TEST(BaseExtension, create ## type ## type ## type ## SubFunction) { \
        typedef ctype (FuncProto)(ctype, ctype); \
        COMPILE_FUNC(type ## _ ## type ## _ ## type ## _SubFunction, FuncProto, f, false); \
        ctype x1=a1; ctype x2=a2; ctype y1=b1; ctype y2=b2; \
        EXPECT_EQ(f(x1,y1), (ctype)(x1-y1)) << "Compiled f(" << x1 << "," << y1 << ") returns " << (ctype)(x1-y1); \
        EXPECT_EQ(f(x2,y2), (ctype)(x2-y2)) << "Compiled f(" << x2 << "," << y2 << ") returns " << (ctype)(x2-y2); \
        ctype min=std::numeric_limits<ctype>::min(); \
        EXPECT_EQ(f(min,x1), (ctype)(min-x1)) << "Compiled f(" << min << "," << x1 << ") returns " << (ctype)(min-x1); \
        EXPECT_EQ(f(min,y1), (ctype)(min-y1)) << "Compiled f(" << min << "," << y1 << ") returns " << (ctype)(min-y1); \
        EXPECT_EQ(f(min,x2), (ctype)(min-x2)) << "Compiled f(" << min << "," << x2 << ") returns " << (ctype)(min-x2); \
        EXPECT_EQ(f(min,y2), (ctype)(min-y2)) << "Compiled f(" << min << "," << y2 << ") returns " << (ctype)(min-y2); \
        ctype max=std::numeric_limits<ctype>::max(); \
        EXPECT_EQ(f(max,x1), (ctype)(max-x1)) << "Compiled f(" << max << "," << x1 << ") returns " << (ctype)(max-x1); \
        EXPECT_EQ(f(max,y1), (ctype)(max-y1)) << "Compiled f(" << max << "," << y1 << ") returns " << (ctype)(max-y1); \
        EXPECT_EQ(f(max,x2), (ctype)(max-x2)) << "Compiled f(" << max << "," << x2 << ") returns " << (ctype)(max-x2); \
        EXPECT_EQ(f(max,y2), (ctype)(max-y2)) << "Compiled f(" << max << "," << y2 << ") returns " << (ctype)(max-y2); \
    }

TESTSUBTYPEFUNC(Int8, int8_t, 0, 1, 1, -1)
TESTSUBTYPEFUNC(Int16, int16_t, 0, 1, 1, -1)
TESTSUBTYPEFUNC(Int32, int32_t, 0, 1, 1, -1)
TESTSUBTYPEFUNC(Int64, int64_t, 0, 1, 1, -1)
TESTSUBTYPEFUNC(Float32, float, 0.0, 1.0, 1.0, -1.0)
TESTSUBTYPEFUNC(Float64, double, 0.0, 1.0, 1.0, -1.0)

SUBTYPEFUNC(Address,Address,Word,)
TEST(BaseExtension, createAddressAddressWordSubFunction) {
    typedef void * (FuncProto)(void *, size_t );
    COMPILE_FUNC(Address_Address_Word_SubFunction, FuncProto, f, false);
    void *p[3]; size_t x;
    x=0;              EXPECT_EQ((uintptr_t)f(p,x), (uintptr_t)p) << "Compiled f(" << p << "," << x << ") returns " << p;
    x=sizeof(void *); EXPECT_EQ((uintptr_t)f(p+1,x), (uintptr_t)p) << "Compiled f(" << p+1 << "," << sizeof(void*) << ") returns " << p;
    x=2*sizeof(void*);EXPECT_EQ((uintptr_t)f(p+2,x), (uintptr_t)p) << "Compiled f(" << p+2 << "," << 2*sizeof(void*) << ") returns " << p;
    x=sizeof(void*);  EXPECT_EQ((uintptr_t)f(p+2,x), (uintptr_t)(p+1)) << "Compiled f(" << p+2 << "," << sizeof(void*) << ") returns " << p+1;
}

SUBTYPEFUNC(Word,Address,Address,)
TEST(BaseExtension, createWordAddressSubFunction) {
    typedef size_t (FuncProto)(void *, void *);
    COMPILE_FUNC(Word_Address_Address_SubFunction, FuncProto, f, false);
    void *p[3]; size_t x;
    x=0;               EXPECT_EQ(f(p,p), x) << "Compiled f(p,0) returns " << 0;
    x=sizeof(void*);   EXPECT_EQ(f(p+1,p), x) << "Compiled f(p+1,p) returns " << (uint8_t*)(p+1)-(uint8_t*)p;
    x=2*sizeof(void*); EXPECT_EQ(f(p+2,p), x) << "Compiled f(p+2,p) returns " << (uint8_t*)(p+2)-(uint8_t *)p;
    x=sizeof(void*);   EXPECT_EQ(f(p+2,p+1), x) << "Compiled f(p+2,p+1) returns " << (uint8_t*)(p+2)-(uint8_t *)(p+1);
}

#define TESTSUBTYPESINVALID(returnType,leftType,rightType) \
    SUBTYPEFUNC(returnType,leftType,rightType,Validity) \
    TEST(BaseExtension, testSubTypesInvalid_ ## leftType ## rightType ) { \
        COMPILE_FUNC_TO_FAIL(returnType ## _ ## leftType ## _ ## rightType ## _SubFunctionValidity , c.lookupExtension<Base::BaseExtension>()->CompileFail_BadInputTypes_Sub, false); \
    }

#define TESTBADSUBTYPES(returnType,leftType,bad1,bad2,bad3,bad4,bad5) \
    TESTSUBTYPESINVALID(returnType,leftType,bad1); \
    TESTSUBTYPESINVALID(returnType,leftType,bad2); \
    TESTSUBTYPESINVALID(returnType,leftType,bad3); \
    TESTSUBTYPESINVALID(returnType,leftType,bad4); \
    TESTSUBTYPESINVALID(returnType,leftType,bad5);

TESTBADSUBTYPES(Int8,Int8,Int16,Int32,Int64,Float32,Float64)
TESTBADSUBTYPES(Int16,Int16,Int8,Int32,Int64,Float32,Float64)
TESTBADSUBTYPES(Int32,Int32,Int8,Int16,Int64,Float32,Float64)
TESTBADSUBTYPES(Int64,Int64,Int8,Int16,Int32,Float32,Float64)
TESTSUBTYPESINVALID(Address,Address,Int8);
TESTSUBTYPESINVALID(Address,Int8,Address);
TESTSUBTYPESINVALID(Address,Address,Int16);
TESTSUBTYPESINVALID(Address,Int16,Address);
#if PLATFORM_32BIT
    TESTSUBTYPESINVALID(Address,Address,Int64);
    TESTSUBTYPESINVALID(Address,Int64,Address);
#else
    TESTSUBTYPESINVALID(Address,Address,Int32);
    TESTSUBTYPESINVALID(Address,Int32,Address);
#endif
TESTSUBTYPESINVALID(Address,Address,Float32);
TESTSUBTYPESINVALID(Address,Float32,Address);
TESTSUBTYPESINVALID(Address,Address,Float64);
TESTSUBTYPESINVALID(Address,Float64,Address);
TESTBADSUBTYPES(Float32,Float32,Int8,Int16,Int32,Int64,Float64)
TESTBADSUBTYPES(Float64,Float64,Int8,Int16,Int32,Int64,Float32)

// Test function that implements an IfThenElse without usint the Else
#define IFTHENFUNCNAME(selectorType) selectorType ## _IfThenFunction

#define IFTHENFUNC(selectorType) \
    BASE_FUNC(IFTHENFUNCNAME(selectorType), "0", "IfThen.cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->Word); \
          ctx->DefineParameter("selector", _bx->selectorType); }, \
        { auto selectorSym=ctx->LookupLocal("selector"); \
          Value * selector = _fx->Load(LOC, b, selectorSym); \
          Base::IfThenElseBuilder bldr = _bx->IfThenElse(LOC, b, selector); { \
              Builder *thenPath = bldr.thenPath(); \
              _fx->Return(LOC, thenPath, _bx->One(LOC, thenPath, _bx->Word)); \
          } \
          _fx->Return(LOC, b, _bx->Zero(LOC, b, _bx->Word)); })

#define TESTIFTHENTYPEFUNC(type,ctype) \
    IFTHENFUNC(type) \
    TEST(BaseExtension, create ## type ## IfThenFunction) { \
        typedef size_t (FuncProto)(ctype); \
        COMPILE_FUNC(IFTHENFUNCNAME(type), FuncProto, f, false); \
        EXPECT_EQ(f(0), 0) << "IfThen(0) returns 0"; \
        EXPECT_EQ(f(1), 1) << "IfThen(1) returns 1"; \
        EXPECT_EQ(f(100), 1) << "IfThen(100) returns 1"; \
        EXPECT_EQ(f(-15), 1) << "IfThen(-15) returns 1"; \
        EXPECT_EQ(f(-127), 1) << "IfThen(-127) return 1"; \
    }

TESTIFTHENTYPEFUNC(Int8,int8_t)
TESTIFTHENTYPEFUNC(Int16,int16_t)
TESTIFTHENTYPEFUNC(Int32,int32_t)
TESTIFTHENTYPEFUNC(Int64,int64_t)
TESTIFTHENTYPEFUNC(Address,size_t)

// Test function that implements an IfThenElse
#define IFTHENELSEFUNCNAME(selectorType) selectorType ## _IfThenElseFunction

#define ELSE

#define IFTHENELSEFUNC(selectorType) \
    BASE_FUNC(IFTHENELSEFUNCNAME(selectorType), "0", "IfThenElse.cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->Word); \
          ctx->DefineParameter("selector", _bx->selectorType); }, \
        { auto selectorSym=ctx->LookupLocal("selector"); \
          Value * selector = _fx->Load(LOC, b, selectorSym); \
          Base::IfThenElseBuilder bldr = _bx->IfThenElse(LOC, b, selector); { \
              Builder *thenPath = bldr.thenPath(); \
              _fx->Return(LOC, thenPath, _bx->One(LOC, thenPath, _bx->Word)); \
          } ELSE { \
              Builder *elsePath = bldr.elsePath(); \
              _fx->Return(LOC, elsePath, _bx->Zero(LOC, elsePath, _bx->Word)); \
          } \
          size_t allOnes=~(size_t)0; \
          LiteralBytes *p = reinterpret_cast<LiteralBytes *>(&allOnes); \
          _fx->Return(LOC, b, _bx->Const(LOC, b, _bx->Word->literal(LOC, comp, p))); })

#define TESTIFTHENELSETYPEFUNC(type,ctype) \
    IFTHENELSEFUNC(type) \
    TEST(BaseExtension, create ## type ## IfThenElseFunction) { \
        typedef size_t (FuncProto)(ctype); \
        COMPILE_FUNC(IFTHENELSEFUNCNAME(type), FuncProto, f, false); \
        EXPECT_EQ(f(0), 0) << "IfThenElse(0) returns 0"; \
        EXPECT_EQ(f(1), 1) << "IfThenElse(1) returns 1"; \
        EXPECT_EQ(f(100), 1) << "IfThenElse(100) returns 1"; \
        EXPECT_EQ(f(-15), 1) << "IfThenElse(-15) returns 1"; \
        EXPECT_EQ(f(-127), 1) << "IfThenElse(-127) return 1"; \
    }

#define TESTIFTHENELSETYPEFUNC_Address \
    IFTHENELSEFUNC(Address) \
    TEST(BaseExtension, create ## Address ## IfThenElseFunction) { \
        typedef size_t (FuncProto)(void *); \
        COMPILE_FUNC(IFTHENELSEFUNCNAME(Address), FuncProto, f, false); \
        EXPECT_EQ(f(0), 0) << "IfThenElse(0) returns 0"; \
        EXPECT_EQ(f((void *)1), 1) << "IfThenElse(1) returns 1"; \
        EXPECT_EQ(f((void *)100), 1) << "IfThenElse(100) returns 1"; \
        EXPECT_EQ(f((void *)0xdeadbeef), 1) << "IfThenElse(100) returns 1"; \
    }

TESTIFTHENELSETYPEFUNC(Int8,int8_t)
TESTIFTHENELSETYPEFUNC(Int16,int16_t)
TESTIFTHENELSETYPEFUNC(Int32,int32_t)
TESTIFTHENELSETYPEFUNC(Int64,int64_t)
TESTIFTHENELSETYPEFUNC_Address

// Test function that implements a for loop
#define FORLOOPFUNCNAME(iterType,initialType,finalType,bumpType,suffix) iterType ## _ ## initialType ## _ ## finalType ## _ ## bumpType ## _ForLoopFunction ## suffix

#define FORLOOPFUNC(iterType,initialType,finalType,bumpType,suffix) \
    BASE_FUNC(FORLOOPFUNCNAME(iterType,initialType,finalType,bumpType,suffix), "0", "ForLoop.cpp", , b, \
        { }, \
        { ctx->DefineReturnType(_bx->Word); \
          ctx->DefineParameter("initial", _bx->initialType); \
          ctx->DefineParameter("final", _bx->finalType); \
          ctx->DefineParameter("bump", _bx->bumpType); \
          ctx->DefineLocal("i", _bx->iterType); \
          ctx->DefineLocal("counter", _bx->Word); }, \
        { auto counterSym=ctx->LookupLocal("counter"); \
          _fx->Store(LOC, b, counterSym, _bx->Zero(LOC, b, counterSym->type())); \
          auto iterVarSym = ctx->LookupLocal("i"); \
          auto initialSym=ctx->LookupLocal("initial"); \
          Value * initial = _fx->Load(LOC, b, initialSym); \
          auto finalSym=ctx->LookupLocal("final"); \
          Value * final = _fx->Load(LOC, b, finalSym); \
          auto bumpSym=ctx->LookupLocal("bump"); \
          Value * bump = _fx->Load(LOC, b, bumpSym); \
          Base::ForLoopBuilder loop = _bx->ForLoopUp(LOC, b, iterVarSym, initial, final, bump); { \
              Builder *loopBody = loop.loopBody(); \
              _fx->addon<Base::BaseFunctionExtensionAddon>()->Increment(LOC, loopBody, counterSym); \
          } \
          _fx->Return(LOC, b, _fx->Load(LOC, b, counterSym)); })

#define FORLOOPTYPEFUNCNAME(iterType) FORLOOPFUNCNAME(iterType,iterType,iterType,iterType,)
#define FORLOOPTYPEFUNC(iterType) FORLOOPFUNC(iterType,iterType,iterType,iterType,)

#define TESTFORLOOPTYPEFUNC(type,ctype) \
    FORLOOPTYPEFUNC(Int32) \
    TEST(BaseExtension, create ## type ## ForLoopFunction) { \
        typedef size_t (FuncProto)(ctype, ctype, ctype); \
        COMPILE_FUNC(FORLOOPTYPEFUNCNAME(type), FuncProto, f, false); \
        EXPECT_EQ(f(0,100,1), 100) << "ForLoopUp(0,100,1) counts 100 iterations"; \
        EXPECT_EQ(f(0,100,2), 50) << "ForLoopUp(0,100,2) counts 50 iterations"; \
        EXPECT_EQ(f(0,100,3), 34) << "ForLoopUp(0,100,3) counts 34 iterations"; \
        EXPECT_EQ(f(1,100,1), 99) << "ForLoopUp(1,100,1) counts 99 iterations"; \
        EXPECT_EQ(f(1,100,3), 33) << "ForLoopUp(1,100,3) counts 33 iterations"; \
        EXPECT_EQ(f(-100,100,1), 200) << "ForLoopUp(-100,100,1) counts 200 iterations"; \
        EXPECT_EQ(f(100,-100,1), 0) << "ForLoopUp(100,-100,1) counts 0 iterations"; \
        EXPECT_EQ(f(100,-100,5), 0) << "ForLoopUp(100,-100,5) counts 0 iterations"; \
        EXPECT_EQ(f(0,0,1), 0) << "ForLoopUp(0,0,1) counts 0 iterations"; \
        EXPECT_EQ(f(-100,-1,1), 99) << "ForLoopUp(-100,-1,1) counts 99 iterations"; \
    }

TESTFORLOOPTYPEFUNC(Int32,int32_t)

#define TESTINVALIDFORLOOP(iterType,initialType,finalType,bumpType) \
    FORLOOPFUNC(iterType,initialType,finalType,bumpType,Validity) \
    TEST(BaseExtension, testForLoopUpTypesInvalid_ ## iterType ## _ ## initialType ## _ ## finalType ## _ ## bumpType ) { \
        COMPILE_FUNC_TO_FAIL(FORLOOPFUNCNAME(iterType,initialType,finalType,bumpType,Validity), c.lookupExtension<Base::BaseExtension>()->CompileFail_BadInputTypes_ForLoopUp, false); \
    }

TESTINVALIDFORLOOP(Int8,Int32,Int32,Int32)
TESTINVALIDFORLOOP(Int32,Int16,Int32,Int32)
TESTINVALIDFORLOOP(Int32,Int64,Int32,Int32)
TESTINVALIDFORLOOP(Int32,Int32,Float32,Int32)
TESTINVALIDFORLOOP(Int32,Int32,Int32,Float64)
