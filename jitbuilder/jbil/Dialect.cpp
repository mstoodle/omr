/*
dialect =
	includes <list of other dialects>
	reduces to <other dialect>

	types
	operations
		visitors
	builders
		visitors

	input validation?
	output validation?
	

new type means operators need to be overriden

every operator:
	generator
	printer
	visitor

operators are like separate classes

Type = NoType, Int8, Int16, Int32, Int64, Float, Double, Address, ...
Service = IlBuilder, FunctionBuilder, TypeDictionary, VirtualMachineRegister, ...
Operation = Add, Sub, Load, DefineLocal, ...

Types can be expanded by dialects (subgroups?)
Operations are expanded by dialects (subgroups?)
Services are composed of dialects that include operations

#include can be used to incorporate new dialects into services
So Services are the primary unit (== class)
Operations are #included into services
Types are specifically #included into TypeDictionary

Dialect is a categorization for operations and types
	an operation is part of a dialect

generating code takes a target dialect (default is native)

standard dialects:
	native
	codegen
	tril
	jbil_ll (only lower level services)
	jbil_hl (higher level services)
*/

#include <stdint.h>

typedef enum OperandType {
   None=-1,
   Boolean,
   String,
   Integer,
   Builder,
   Case,
   Value
} OperandType;


#define Dialect(name) Dialect_##name
const int64_t Dialect(native) = 1;
#define DefineDialect(name,builton) \
	const int64_t Dialect(name)=2*Dialect(builton); \
	static_assert((Dialect(builton) != ((uint64_t)1) << 64), "ran out of dialect identifiers")

// Let's define the JitBuilder dialect
DefineDialect(codegen, native);
DefineDialect(tril, codegen);
DefineDialect(jbil_ll, tril);
DefineDialect(jbil, jbil_ll);


#define DefineService(dialect, name) \
	public class name_##dialect

#define ExtendService(dialect, name, extends) \
	public class name_##dialect : public name_##extends

#define type_NoType void
#define type_Int8 int8_t
#define type_Int16 int16_t
#define type_Int32 int32_t
#define type_Int64 int64_t
#define type_Float float
#define type_Double double
#define type_Address uintptr_t

#define DefineOperation0(name, returnType) \
	type_##returnType name()

DefineService(jbil_ll, IlBuilder) {
public:

   DefineOperation(IfCmpLessThan, Builder, target, Value, left, Value, right) {
      .addOperand(target, Builder)
      .addOperand(left, Value)
      .addOperand(right, Value)
      .addGenerator(generator_IfCmpLessThan)
      .addValidator(validator_IfCmpLessThan);

};

#define OperationHandlerName(service,operation) service::operation

// could templates clean this up for C++ ?
#define DefineOperation3(service, name, retType, p1, p2, p3) \
type_##retType \
OperationHandlerName(service,name)(type_##p1Type p1Name, type_##p2Type p2Name, type_##p3Type p3Name) { \
   Operation::Creator c; \
   add(c.set(a##name).setParent(this) \
        .addOperand(p1Name) \
        .addOperand(p2Name) \
        .addOperand(p3Name) \
        .addGenerator(Generator_##service##_##name) \
        .addValidator(Validator_##service##_##name) \
        .create(); \
}

ExtendService(jbil, IlBuilder, jbil_ll) {
public:

#define DefineOperation(service, name, validator, generator, ...)
   DefineOperation(IlBuilder, ForLoop, validator_ForLoop, generator_ForLoop,
                   Arg(countsUp, Boolean),
                   Arg(indVar, String),
                   Arg(loopBody, Builder),
                   Arg(loopContinue, Builder),
                   Arg(loopBreak, Builder),
                   Arg(initial, Value),
                   Arg(end, Value),
                   Arg(increment, Value))

   DefineOperation(ForLoop)
      .addOperand(countsUp, Boolean)
      .addOperand(indVar, String)
      .addOperand(loopBody, Builder)
      .addOperand(loopContinue, Builder)
      .addOperand(loopBreak, Builder)
      .addOperand(initial, Value)
      .addOperand(end, Value)
      .addOperand(increment, Value)
      .addGenerator(generator_ForLoop)
      .addValidator(validator_ForLoop);

};

void
Builder::ForLoopUp(const char *loopVar, Builder & body, Value & initial, Value & end, Value & bump)
   {
   Operation::Creator c;
   add(c.set(aForLoopUp).setParent(this)
        .operand(loopVar)
        .operand(body)
        .operand(initial.read(*this))
        .operand(end.read(*this))
        .operand(bump.read(*this))
        .create());
   }



#if 0 

DefineOperation("IfCmpLessThan")
   .addOperand("target", "Builder")
   .addOperand("left", "Value")
   .addOperand("right", "Value")
   .addGenerator(g)
   .addPrinter(p)
   .addValidator(v);

DefineType(Complex)
   .addValidator(v)
   .addPrinter(f);

DefineDialect("JitBuilderBase")
   .includes(OMR)
   .addService(TypeDictionary)
   .addService(FunctionBuilder)
   .addService(IlBuilder)
   .addService(ThunkBuilder)
   ...

DefineService(TypeDictionary)
   .addType(NoType)
   .addType(Int8)
   .addType(Int16)
   .addType(Int32)
   .addType(Int64)
   .addType(Float)
   .addType(Double)
   .addType(Address)
   .addOperation(DefineStruct)
   ...


DefineService(IlBuilder)
   .addOperation(Add)
   .addOperation(Sub)
   .addOperation(Mul)
   .addOperation(IndexAt)
   .addOperation(Load)
   .addOperation(LoadAt)
   .addOperation(Store)
   .addOperation(StoreAt)
   .addOperation(IfCmpLessThan)
   .addOperation(IfCmpGreaterThan)
   .addOperation(ForLoop)
   .addOperation(IfThenElse)
   .addOperation(Switch)
   ...

DefineOperation(ForLoop)
   .addOperand(countsUp, Boolean)
   .addOperand(indVar, String)
   .addOperand(loopBody, Builder)
   .addOperand(loopContinue, Builder)
   .addOperand(loopBreak, Builder)
   .addOperand(initial, Value)
   .addOperand(increment, Value)
   .addOperand(end, Value)
   .addVisitor(Generator)
   .addVisitor(Validator,v);

DefineVisitor(Generator, ForLoop) {
   Builder &b = GeneratedBuilder;
   bool countsUp = Operand("countsUp"); //_booleans[0];
   std::string &indVar = Operand("indVar"); //*(_strings[0]);
   Value &initial = Operand("initial"); //*(_operands[0]);
   Value &end = Operand("end"); //*(_operands[1]);
   Value &increment = Operand("increment"); //*(_operands[2]);
   Builder &loopCode = Operand("loopCode"); //*(_builders[0]);
   Builder *loopContinue=Operand("loopContinue"); // if (_builders.size() > 1) loopContinue = (_builders[1]);
   Builder *breakBuilder=Operand("loopBreak"); // if (_builders.size() > 2) breakBuilder = (_builders[2]);

   Builder & loopBody = b.OrphanBuilder();

   b.Store(indVar, initial);

   if (countsUp)
      b.IfCmpLessThan(loopBody, b,Load(indVar), end);
   else
      b.IfCmpGreaterThan(loopBody, b.Load(indVar), end);

   loopBody.AppendBuilder(loopCode);
   loopBody.AppendBuilder(loopContinue);

   if (countsUp)
      {
      loopContinue.Store(indVar,
      loopContinue.   Add(
      loopContinue.      Load(indVar),
                         increment));
      loopContinue.IfCmpLessThan(loopBody, 
      loopContinue.   Load(indVar),
                      end);
      }
   else
      {
      loopContinue.Store(indVar,
      loopContinue.   Sub(
      loopContinue.      Load(indVar),
                         increment));
      loopContinue.IfCmpGreaterThan(loopBody, 
      loopContinue.   Load(indVar),
                      end);
      }

   if (breakBuilder)
      {
      b.AppendBuilder(breakBuilder);
      }

   return DIALECT_LLJB;
}

DefineService(FunctionBuilder)
   .addOperation(DefineName)
   .addOperation(DefineLocal)
   ...

DefineOperation(DefineName)
   .addOperand(String)
   .addOperand(Type()

DefineDialect("JitBuilder")
   .addService(BytecodeBuilder)
   .addService(VirtualMachineState)
   .addService(VirtualMachineRegister)
   .addService(VirtualMachineRegisterInStruct)
   .addService(VirtualMachineOperandStack)
   .addService(VirtualMachineOperandArray)
#endif
