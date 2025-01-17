/*******************************************************************************
 * Copyright (c) 2016, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/


#include <cstddef>
#include <cstring>
#include <dlfcn.h>
#include <errno.h>
#include <iostream>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "OperandStackTest.hpp"

using std::cout;
using std::cerr;

using namespace OMR::JB2;

typedef struct Thread {
    int pad;
    STACKVALUECTYPE *sp;
} Thread;

class TestState : public VM::VirtualMachineState {
    JBALLOC_NO_DESTRUCTOR(TestState, NoAllocationCategory)

public:

    TestState(MEM_LOCATION(a), VM::VMExtension *vmx)
        : VM::VirtualMachineState(MEM_PASSLOC(a), vmx, CLASSKIND(TestState,Extensible)),
        _stack(NULL),
        _stackTop(NULL) {

    }

    TestState(MEM_LOCATION(a), VM::VMExtension *vmx, VM::VirtualMachineOperandStack *stack, VM::VirtualMachineRegister *stackTop)
        : VM::VirtualMachineState(MEM_PASSLOC(a), vmx, CLASSKIND(TestState,Extensible)),
        _stack(stack),
        _stackTop(stackTop) {

    }
    virtual ~TestState() {
        Allocator *mem = allocator();
        if (_stackTop)
            delete _stackTop;
        if (_stack)
            delete _stack;
    }

    virtual void Commit(LOCATION, Builder *b) {
        _stack->Commit(PASSLOC, b);
        _stackTop->Commit(PASSLOC, b);
    }

    virtual void Reload(LOCATION, Builder *b) {
        _stack->Reload(PASSLOC, b);
        _stackTop->Reload(PASSLOC, b);
    }

    virtual VM::VirtualMachineState *MakeCopy(LOCATION, Builder *b) {
        Allocator *mem = allocator();
        TestState *newState = new (mem) TestState(MEM_PASSLOC(mem), vmx(),
                                                  _stack->MakeCopy(PASSLOC, b)->refine<VM::VirtualMachineOperandStack>(),
                                                  _stackTop->MakeCopy(PASSLOC, b)->refine<VM::VirtualMachineRegister>());
        return newState;
    }

    virtual void MergeInto(LOCATION, VM::VirtualMachineState *other, Builder *b) {
        TestState *otherState = other->refine<TestState>();
        _stack->MergeInto(PASSLOC, otherState->_stack, b);
        _stackTop->MergeInto(PASSLOC, otherState->_stackTop, b);
    }

    VM::VirtualMachineOperandStack * _stack;
    VM::VirtualMachineRegister * _stackTop;

    SUBCLASS_KINDSERVICE_DECL(Extensible, TestState);
};

SUBCLASS_KINDSERVICE_IMPL(TestState,"TestState",VirtualMachineState,Extensible);

static bool verbose = false;
static int32_t numFailingTests = 0;
static int32_t numPassingTests = 0;
static STACKVALUECTYPE **verifySP = NULL;
static STACKVALUECTYPE expectedResult12Top = -1;
static char * result12Operator;
static Thread thread;
static bool useThreadSP = false;

static void
setupResult12Equals() {
    expectedResult12Top = 11;
    result12Operator = (char *)"==";
}

static void
setupResult12NotEquals() {
    expectedResult12Top = 99;
    result12Operator = (char *)"!=";
}

int
main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--verbose") == 0)
        verbose = true;

    if (verbose) cout << "Step 0: load" << OMR_JB2_CORELIB << "\n";
    void *handle = dlopen(OMR_JB2_CORELIB, RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        return -1;
    }

    if (verbose) cout << "Step 1: Create a Compiler\n";
    Compiler compiler("OperandStackTests");
    compiler.config()->setTraceCodeGenerator(true);

    if (verbose) cout << "Step 2: lookup and load extensions (core, JB, Base, Func, and VM)\n";
    CoreExtension *cx = compiler.lookupExtension<CoreExtension>();
    jbgen::JBExtension *jx = compiler.loadExtension<jbgen::JBExtension>(LOC);
    //omrgen::OMRExtension *omr = compiler.loadExtension<omrgen::OMRExtension>(LOC);
    Base::BaseExtension *bx = compiler.loadExtension<Base::BaseExtension>(LOC);
    Func::FunctionExtension *fx = compiler.loadExtension<Func::FunctionExtension>(LOC);
    VM::VMExtension *vmx = compiler.loadExtension<VM::VMExtension>(LOC);
    assert(vmx);

    if (verbose) cout << "Step 3: Create Function object\n";
    OperandStackTestFunction *pointerFunction = new (compiler.mem()) OperandStackTestFunction(MEM_LOC(compiler.mem()), vmx);

    if (verbose) cout << "Step 4: Set up logging configuration\n";
    TextLogger logger(std::cout, String("    "));
    TextLogger *wrt = (verbose) ? &logger : NULL;
    
    if (verbose) cout << "Step 5: compile function\n";
    const StrategyID codegenStrategy = cx->strategyCodegen; \
    CompiledBody * body = fx->compile(LOC, pointerFunction, codegenStrategy, wrt); \
    
    if (body->rc() != compiler.CompileSuccessful) {
        cout << "Compile failed: " << compiler.returnCodeName(body->rc()).c_str() << "\n";
        cout << compiler.errorCondition()->message().c_str();
        exit(-1);
    }

    if (verbose) cout << "Step 6: invoke compiled function and print results\n";
    typedef void (OperandStackTestProto)();
    OperandStackTestProto *ptrTest = body->nativeEntryPoint<OperandStackTestProto>();
    verifySP = pointerFunction->getSPPtr();
    setupResult12Equals();
    ptrTest();

    if (verbose) cout << "Step 7: Set up operand stack tests using a Thread structure\n";
    OperandStackTestUsingStructFunction *threadFunction = new (compiler.mem()) OperandStackTestUsingStructFunction(MEM_LOC(compiler.mem()), vmx);

    if (verbose) cout << "Step 8: compile function\n";
    body = fx->compile(LOC, threadFunction, codegenStrategy, wrt);
    if (body->rc() != compiler.CompileSuccessful) {
        cout << "Compile failed: " << compiler.returnCodeName(body->rc()).c_str() << "\n";
        cout << compiler.errorCondition()->message().c_str();
        exit(-1);
    }

    if (verbose) cout << "Step 9: invoke compiled code and print results\n";
    typedef void (OperandStackTestUsingStructProto)(Thread *thread);
    OperandStackTestUsingStructProto *threadTest = body->nativeEntryPoint<OperandStackTestUsingStructProto>();

    useThreadSP = true;
    verifySP = &thread.sp;
    setupResult12NotEquals();
    threadTest(&thread);

    cout << "Number passing tests: " << numPassingTests << "\n";
    cout << "Number failing tests: " << numFailingTests << "\n";

    if (numFailingTests == 0)
        cout << "ALL PASS\n";
    else
        cout << "SOME FAILURES\n";
}


STACKVALUECTYPE *OperandStackTestFunction::_realStack = NULL;
STACKVALUECTYPE *OperandStackTestFunction::_realStackTop = _realStack - 1;
int32_t OperandStackTestFunction::_realStackSize = -1;

void
OperandStackTestFunction::createStack() {
    int32_t stackSizeInBytes = _realStackSize * sizeof(STACKVALUECTYPE);
    _realStack = (STACKVALUECTYPE *) malloc(stackSizeInBytes);
    _realStackTop = _realStack - 1;
    thread.sp = _realStackTop;
    memset(_realStack, 0, stackSizeInBytes);
}

STACKVALUECTYPE *
OperandStackTestFunction::moveStack() {
    int32_t stackSizeInBytes = _realStackSize * sizeof(STACKVALUECTYPE);
    STACKVALUECTYPE *newStack = (STACKVALUECTYPE *) malloc(stackSizeInBytes);
    int32_t delta = 0;
    if (useThreadSP)
        delta = thread.sp - _realStack;
    else
        delta = _realStackTop - _realStack;
    memcpy(newStack, _realStack, stackSizeInBytes);
    memset(_realStack, 0xFF, stackSizeInBytes);
    free(_realStack);
    _realStack = newStack;
    _realStackTop = _realStack + delta;
    thread.sp = _realStackTop;

    return _realStack - 1;
}

void
OperandStackTestFunction::freeStack() {
    memset(_realStack, 0xFF, _realStackSize * sizeof(STACKVALUECTYPE));
    free(_realStack);
    _realStack = NULL;
    _realStackTop = NULL;
    thread.sp = NULL;
}

static void FailingTest() {
    numFailingTests++;
}

static void PassingTest() {
    numPassingTests++;
}

#define REPORT1(c,n,v)         { if (c) { PassingTest(); if (verbose) cout << "Pass\n"; } else { FailingTest(); if (verbose) cout << "Fail: " << (n) << " is " << (v) << "\n"; } }
#define REPORT2(c,n1,v1,n2,v2) { if (c) { PassingTest(); if (verbose) cout << "Pass\n"; } else { FailingTest(); if (verbose) cout << "Fail: " << (n1) << " is " << (v1) << ", " << (n2) << " is " << (v2) << "\n"; } }

// Result 0: empty stack even though Push has happened
void
verifyResult0() {
    if (verbose) cout << "Push(1)  [ no commit ]\n";
    OperandStackTestFunction::verifyStack("0", -1, 0);
}

void
verifyResult1() {
    if (verbose) cout << "Commit(); Top()\n";
    OperandStackTestFunction::verifyStack("1", 0, 1, 1);
}

void
verifyResult2(STACKVALUECTYPE top) {
    if (verbose) cout << "Push(2); Push(3); Top()   [ no commit]\n";
    if (verbose) cout << "\tResult 2: top value == 3: ";
    REPORT1(top == 3, "top", top);

    OperandStackTestFunction::verifyStack("2", 0, 1, 1);
}

void
verifyResult3(STACKVALUECTYPE top) {
    if (verbose) cout << "Commit(); Top()\n";
    if (verbose) cout << "\tResult 3: top value == 3: ";
    REPORT1(top == 3, "top", top);

    OperandStackTestFunction::verifyStack("3", 2, 3, 1, 2, 3);
}

void
verifyResult4(STACKVALUECTYPE popValue) {
    if (verbose) cout << "Pop()    [ no commit]\n";
    if (verbose) cout << "\tResult 4: pop value == 3: ";
    REPORT1(popValue == 3, "popValue", popValue);

    OperandStackTestFunction::verifyStack("4", 2, 3, 1, 2, 3);
}

void
verifyResult5(STACKVALUECTYPE popValue) {
    if (verbose) cout << "Pop()    [ no commit]\n";
    if (verbose) cout << "\tResult 5: pop value == 2: ";
    REPORT1(popValue == 2, "popValue", popValue);

    OperandStackTestFunction::verifyStack("5", 2, 3, 1, 2, 3);
}

void
verifyResult6(STACKVALUECTYPE top) {
    if (verbose) cout << "Push(Add(popValue1, popValue2)); Commit(); Top()\n";
    if (verbose) cout << "\tResult 6: top == 5: ";
    REPORT1(top == 5, "top", top);

    OperandStackTestFunction::verifyStack("6", 2, 2, 1, 5);
}

void
verifyResult7() {
    if (verbose) cout << "Drop(2); Commit(); [ empty stack ]\n";
    OperandStackTestFunction::verifyStack("7", 2, 0);
}

void
verifyResult8(STACKVALUECTYPE pick) {
    if (verbose) cout << "Push(5); Push(4); Push(3); Push(2); Push(1); Commit(); Pick(3)\n";
    if (verbose) cout << "\tResult 8: pick == 4: ";
    REPORT1(pick == 4, "pick", pick);

    OperandStackTestFunction::verifyStack("8", 2, 0);
}

void
verifyResult9(STACKVALUECTYPE top) {
    if (verbose) cout << "Drop(2); Top()\n";
    if (verbose) cout << "\tResult 9: top == 3: ";
    REPORT1(top == 3, "top", top);

    OperandStackTestFunction::verifyStack("9", 2, 0);
}

void
verifyResult10(STACKVALUECTYPE pick) {
    if (verbose) cout << "Dup(); Pick(2)\n";
    if (verbose) cout << "\tResult 10: pick == 4: ";
    REPORT1(pick == 4, "pick", pick);
 
    OperandStackTestFunction::verifyStack("10", 2, 0);
}

void
verifyResult11() {
    if (verbose) cout << "Commit();\n";
    OperandStackTestFunction::verifyStack("11", 3, 4, 5, 4, 3, 3);
}

void
verifyResult12(STACKVALUECTYPE top) {
    if (verbose) cout << "Pop(); Pop(); if (3 " << result12Operator << " 3) { Push(11); } else { Push(99); } Commit(); Top();\n";
    if (verbose) cout << "\tResult 12: top == " << expectedResult12Top << ": ";
    REPORT1(top == expectedResult12Top, "top", top);
    OperandStackTestFunction::verifyStack("11", 3, 3, 5, 4, expectedResult12Top);
}

// used to compare expected values and report fail it not equal
void
verifyValuesEqual(STACKVALUECTYPE v1, STACKVALUECTYPE v2) { 
    REPORT2(v1 == v2, "verifyValuesEqual v1", v1, "verifyValuesEqual v2", v2); 
}

// take the arguments from the stack and modify them
void
modifyTop3Elements(int32_t amountToAdd) {
    if (verbose) cout << "Push();Push();Push() - modify elements passed in real stack and return";
    STACKVALUECTYPE *realSP = *verifySP; 
    REPORT1(realSP[0]== 3, "modifyTop3Elements realSP[0]", realSP[0]); 
    REPORT1(realSP[-1]== 2, "modifyTop3Elements realSP[-1]", realSP[-1]); 
    REPORT1(realSP[-2]== 1, "modifyTop3Elements realSP[-2]", realSP[-2]); 
    realSP[0] += amountToAdd;
    realSP[-1] += amountToAdd;
    realSP[-2] += amountToAdd;  
}




bool
OperandStackTestFunction::verifyUntouched(int32_t maxTouched) {
    for (int32_t i=maxTouched+1;i < _realStackSize;i++) 
        if (_realStack[i] != 0)
            return false;
    return true;
}

void
OperandStackTestFunction::verifyStack(const char *step, int32_t max, int32_t num, ...) {

    STACKVALUECTYPE *realSP = *verifySP;

    if (verbose) cout << "\tResult " << step << ": realSP-_realStack == " << (num-1) << ": ";
    REPORT2((realSP-_realStack) == (num-1), "_realStackTop-_realStack", (realSP-_realStack), "num-1", (num-1));

    va_list args;
    va_start(args, num);
    for (int32_t a=0;a < num;a++) {
        STACKVALUECTYPE val = va_arg(args, STACKVALUECTYPE);
        if (verbose) cout << "\tResult " << step << ": _realStack[" << a << "] == " << val << ": ";
        REPORT2(_realStack[a] == val, "_realStack[a]", _realStack[a], "val", val);
    }
    va_end(args);

    if (verbose) cout << "\tResult " << step << ": upper stack untouched: ";
    REPORT1(verifyUntouched(max), "max", max);
}


OperandStackTestFunction::OperandStackTestFunction(MEM_LOCATION(a), VM::VMExtension *vmx)
    : VM::VMFunction(MEM_PASSLOC(a), vmx->compiler(), vmx)
    , _cx(vmx->compiler()->coreExt())
    , _bx(vmx->bx())
    , _fx(vmx->fx())
    , _vmx(vmx) {

    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);
    DefineName("OperandStackTest");
}

bool
OperandStackTestFunction::buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
    IR *ir = comp->ir();
    const Type *NoType = ir->NoType;
    ctx->DefineReturnType(NoType);

    _realStackSize = 32;
    _valueType = _bx->STACKVALUETYPE(ir);
    const Type *pValueType = _bx->PointerTo(LOC, _valueType);

    _createStack = ctx->DefineFunction(LOC, comp, "createStack", "0", "0", (void *)&OperandStackTestFunction::createStack, NoType, 0);
    _moveStack = ctx->DefineFunction(LOC, comp, "moveStack", "0", "0", (void *)&OperandStackTestFunction::moveStack, pValueType, 0);
    _freeStack = ctx->DefineFunction(LOC, comp, "freeStack", "0", "0", (void *)&OperandStackTestFunction::freeStack, NoType, 0);
    _verifyResult0 = ctx->DefineFunction(LOC, comp, "verifyResult0", "0", "0", (void *)&verifyResult0, NoType, 0);
    _verifyResult1 = ctx->DefineFunction(LOC, comp, "verifyResult1", "0", "0", (void *)&verifyResult1, NoType, 0);
    _verifyResult2 = ctx->DefineFunction(LOC, comp, "verifyResult2", "0", "0", (void *)&verifyResult2, NoType, 1, _valueType);
    _verifyResult3 = ctx->DefineFunction(LOC, comp, "verifyResult3", "0", "0", (void *)&verifyResult3, NoType, 1, _valueType);
    _verifyResult4 = ctx->DefineFunction(LOC, comp, "verifyResult4", "0", "0", (void *)&verifyResult4, NoType, 1, _valueType);
    _verifyResult5 = ctx->DefineFunction(LOC, comp, "verifyResult5", "0", "0", (void *)&verifyResult5, NoType, 1, _valueType);
    _verifyResult6 = ctx->DefineFunction(LOC, comp, "verifyResult6", "0", "0", (void *)&verifyResult6, NoType, 1, _valueType);
    _verifyResult7 = ctx->DefineFunction(LOC, comp, "verifyResult7", "0", "0", (void *)&verifyResult7, NoType, 0);
    _verifyResult8 = ctx->DefineFunction(LOC, comp, "verifyResult8", "0", "0", (void *)&verifyResult8, NoType, 1, _valueType);
    _verifyResult9 = ctx->DefineFunction(LOC, comp, "verifyResult9", "0", "0", (void *)&verifyResult9, NoType, 1, _valueType);
    _verifyResult10 = ctx->DefineFunction(LOC, comp, "verifyResult10", "0", "0", (void *)&verifyResult10, NoType, 1, _valueType);
    _verifyResult11 = ctx->DefineFunction(LOC, comp, "verifyResult11", "0", "0", (void *)&verifyResult11, NoType, 0);
    _verifyResult12 = ctx->DefineFunction(LOC, comp, "verifyResult12", "0", "0", (void *)&verifyResult12, NoType, 1, _valueType);
    _verifyValuesEqual = ctx->DefineFunction(LOC, comp, "verifyValuesEqual", "0", "0", (void *)&verifyValuesEqual, NoType, 2, _valueType, _valueType);
    _modifyTop3Elements = ctx->DefineFunction(LOC, comp, "modifyTop3Elements", "0", "0", (void *)&modifyTop3Elements, NoType, 1, _valueType);

    return true;
}

// convenience macros
#define STACK(b)           (((b)->addon<VM::VMBuilderAddon>()->vmState()->refine<TestState>())->_stack)
#define STACKTOP(b)        (((b)->addon<VM::VMBuilderAddon>()->vmState()->refine<TestState>())->_stackTop)
#define COMMIT(b)          ((b)->addon<VM::VMBuilderAddon>()->vmState()->Commit(LOC, b))
#define RELOAD(b)          ((b)->addon<VM::VMBuilderAddon>()->vmState()->Reload(LOC, b))
#define UPDATESTACK(b,s)   (STACK(b)->UpdateStack(LOC, b, s))
#define PUSH(b,v)          (STACK(b)->Push(v))
#define POP(b)             (STACK(b)->Pop())
#define TOP(b)             (STACK(b)->Top())
#define DUP(b)             (STACK(b)->Dup())
#define DROP(b,d)          (STACK(b)->Drop(d))
#define PICK(b,d)          (STACK(b)->Pick(d))

Builder *
OperandStackTestFunction::testStack(Builder *b, bool useEqual) {
    STACKVALUECTYPE one=1;
    Literal *lv1 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&one));
    PUSH(b, _bx->Const(LOC, b, lv1));
    _fx->Call(LOC, b, _verifyResult0);

    COMMIT(b);
    _fx->Call(LOC, b, _verifyResult1);

    STACKVALUECTYPE two=2;
    Literal *lv2 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&two));
    PUSH(b, _bx->Const(LOC, b, lv2));

    STACKVALUECTYPE three=3;
    Literal *lv3 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&three));
    PUSH(b, _bx->Const(LOC, b, lv3));
    _fx->Call(LOC, b, _verifyResult2, TOP(b));

    COMMIT(b);
    Value *newStack = _fx->Call(LOC, b, _moveStack);
    UPDATESTACK(b, newStack);
    _fx->Call(LOC, b, _verifyResult3, TOP(b));

    Value *val1 = POP(b);
    _fx->Call(LOC, b, _verifyResult4, val1);

    Value *val2 = POP(b);
    _fx->Call(LOC, b, _verifyResult5, val2);

    Value *sum = _bx->Add(LOC, b, val1, val2);
    PUSH(b, sum);
    COMMIT(b);
    newStack = _fx->Call(LOC, b, _moveStack);
    UPDATESTACK(b, newStack);
    _fx->Call(LOC, b, _verifyResult6, TOP(b));

    DROP(b, 2);
    COMMIT(b);
    _fx->Call(LOC, b, _verifyResult7);

    STACKVALUECTYPE four=4;
    Literal *lv4 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&four));
    STACKVALUECTYPE five=5;
    Literal *lv5 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&five));

    PUSH(b, _bx->Const(LOC, b, lv5));
    PUSH(b, _bx->Const(LOC, b, lv4));
    PUSH(b, _bx->Const(LOC, b, lv3));
    PUSH(b, _bx->Const(LOC, b, lv2));
    PUSH(b, _bx->Const(LOC, b, lv1));
    _fx->Call(LOC, b, _verifyResult8, PICK(b, 3));

    DROP(b, 2);
    _fx->Call(LOC, b, _verifyResult9, TOP(b));

    DUP(b);
    _fx->Call(LOC, b, _verifyResult10, PICK(b, 2));

    COMMIT(b);
    newStack = _fx->Call(LOC, b, _moveStack);
    UPDATESTACK(b, newStack);
    _fx->Call(LOC, b, _verifyResult11);

    Builder *thenBB = _vmx->OrphanBuilder(LOC, b, 1, 1, NULL, String("BCI_then"));
    Builder *elseBB = _vmx->OrphanBuilder(LOC, b, 2, 1, NULL, String("BCI_else"));
    Builder *mergeBB = _vmx->OrphanBuilder(LOC, b, 3, 1, NULL, String("BCI_merge"));

    Value *v1 = POP(b);
    Value *v2 = POP(b);
    if (useEqual)
        _vmx->IfCmpEqual(LOC, b, thenBB, v1, v2);
    else
        _vmx->IfCmpNotEqual(LOC, b, thenBB, v1, v2);
    _vmx->Goto(LOC, b, elseBB);

    STACKVALUECTYPE eleven=11;
    Literal *lv11 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&eleven));
    PUSH(thenBB, _bx->Const(LOC, thenBB, lv11));
    _vmx->Goto(LOC, thenBB, mergeBB);

    STACKVALUECTYPE ninetynine=99;
    Literal *lv99 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&ninetynine));
    PUSH(elseBB, _bx->Const(LOC, elseBB, lv99));
    _vmx->Goto(LOC, elseBB, mergeBB);

    COMMIT(mergeBB);
    newStack = _fx->Call(LOC, mergeBB, _moveStack);
    UPDATESTACK(mergeBB, newStack);
    _fx->Call(LOC, mergeBB, _verifyResult12, TOP(mergeBB));
 
    STACKVALUECTYPE amountToAdd = 10;
    Literal *lvAmount = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&amountToAdd));

    // Reload test. Call a routine that modifies stack elements passed to it. 
    // Test by reloading and test the popped values
    PUSH(mergeBB, _bx->Const(LOC, mergeBB, lv1));
    PUSH(mergeBB, _bx->Const(LOC, mergeBB, lv2));
    PUSH(mergeBB, _bx->Const(LOC, mergeBB, lv3));  
    COMMIT(mergeBB); 
    _fx->Call(LOC, mergeBB, _modifyTop3Elements, _bx->Const(LOC, mergeBB, lvAmount));  
    RELOAD(mergeBB);

    Value *modifiedStackElement = POP(mergeBB);
    STACKVALUECTYPE amountPlus3 = 3+amountToAdd;
    Literal *lvAmountPlus3 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&amountPlus3));
    Value *expected =  _bx->Const(LOC, mergeBB, lvAmountPlus3); 
    _fx->Call(LOC, mergeBB, _verifyValuesEqual, modifiedStackElement, expected);  

    modifiedStackElement = POP(mergeBB);
    STACKVALUECTYPE amountPlus2 = 2+amountToAdd;
    Literal *lvAmountPlus2 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&amountPlus2));
    expected = _bx->Const(LOC, mergeBB, lvAmountPlus2);
    _fx->Call(LOC, mergeBB, _verifyValuesEqual, modifiedStackElement, expected);  

    modifiedStackElement = POP(mergeBB);
    STACKVALUECTYPE amountPlus1 = 1+amountToAdd;
    Literal *lvAmountPlus1 = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&amountPlus1));
    expected =  _bx->Const(LOC, mergeBB, lvAmountPlus1);
    _fx->Call(LOC, mergeBB, _verifyValuesEqual, modifiedStackElement, expected);  

    _fx->Call(LOC, mergeBB, _freeStack);

    _fx->Return(LOC, mergeBB);

    return mergeBB;
}

bool
OperandStackTestFunction::buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
    IR *ir = comp->ir();
    const Base::PointerType *pElementType = _bx->PointerTo(LOC, _bx->PointerTo(LOC, _bx->STACKVALUETYPE(ir)));

    Builder *entry = scope->entryPoint<BuilderEntry>()->builder();
    _fx->Call(LOC, entry, _createStack);

    Value *realStackTopAddress = _bx->ConstPointer(LOC, entry, pElementType, &_realStackTop);
    Allocator *mem = comp->mem();
    VM::VirtualMachineRegister *stackTop = new (mem) VM::VirtualMachineRegister(MEM_LOC(mem), _vmx, "SP", comp, realStackTopAddress);
    VM::VirtualMachineOperandStack *stack = new (mem) VM::VirtualMachineOperandStack(MEM_LOC(mem), _vmx, comp, 1, stackTop, _bx->STACKVALUETYPE(ir));

    TestState *vmState = new (mem) TestState(MEM_LOC(mem), _vmx, stack, stackTop);

    Builder *bb = _vmx->OrphanBuilder(PASSLOC, entry, 0, 0, scope, String("entry"));
    bb->addon<VM::VMBuilderAddon>()->setVMState(vmState); // ownership passed to bb so we should never delete vmState ourselves
    _bx->Goto(LOC, entry, bb);

    testStack(bb, true);

    return true;
}




OperandStackTestUsingStructFunction::OperandStackTestUsingStructFunction(MEM_LOCATION(a), VM::VMExtension *vmx)
    : OperandStackTestFunction(MEM_PASSLOC(a), vmx) {
    }


bool
OperandStackTestUsingStructFunction::buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
    IR *ir = comp->ir();
    OperandStackTestFunction::buildContext(PASSLOC, comp, scope, ctx);

    Base::StructTypeBuilder builder(_bx, comp);
    builder.setName("Thread")
           ->addField("sp", _bx->PointerTo(LOC, _bx->STACKVALUETYPE(ir)), 8*offsetof(Thread, sp));
    _threadType = builder.create(LOC);
    _spField = _threadType->LookupField("sp");

    _threadParam = ctx->DefineParameter("thread", _bx->PointerTo(LOC, _threadType));

    return true;
}

bool
OperandStackTestUsingStructFunction::buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
    IR *ir = comp->ir();
    Builder *entry = scope->entryPoint<BuilderEntry>()->builder();
    _fx->Call(LOC, entry, _createStack);

    Allocator *mem = comp->mem();
    VM::VirtualMachineRegisterInStruct *stackTop = new (mem) VM::VirtualMachineRegisterInStruct(MEM_LOC(mem), _vmx, "SP", comp, _spField, _threadParam);
    VM::VirtualMachineOperandStack *stack = new (mem) VM::VirtualMachineOperandStack(MEM_LOC(mem), _vmx, comp, 1, stackTop, _bx->STACKVALUETYPE(ir));

    TestState *vmState = new (mem) TestState(MEM_LOC(mem), _vmx, stack, stackTop);
    Builder *bb = _vmx->OrphanBuilder(PASSLOC, entry, 0, 0, scope, String("entry"));
    bb->addon<VM::VMBuilderAddon>()->setVMState(vmState); // ownership transferred to bb so should never delete it ourselves
    _bx->Goto(LOC, entry, bb);

    testStack(bb, false);

    return true;
}
