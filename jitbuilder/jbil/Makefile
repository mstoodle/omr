OMRDIR=../..
JITB=jitbuilder
LIBJITB=lib$(JITB).a
LIBJITBDIR=$(OMRDIR)/build/jitbuilder

JITB2=jitbuilder2
LIBJITB2=lib$(JITB2).a

all: matmult testTypeGraph cmatmult

matmult: MatMult.o JitBuilder.hpp PrettyPrinter.hpp $(LIBJITB2)
	g++ -g -o matmult -L. -l$(JITB2) -L$(LIBJITBDIR) -l$(JITB) MatMult.o

testTypeGraph: testTypeGraph.o
	g++ -g -o testTypeGraph -L. -l$(JITB2) testTypeGraph.o

cmatmult:
	cd ComplexMatMult; make cmatmult

$(LIBJITB2): Action.o Operation.o OperationBase.o Builder.o BuilderBase.o CodeGenerator.o DialectReducer.o FunctionBuilder.o Location.o Object.o Value.o Transformer.o Type.o TypeDictionary.o TypeGraph.o Visitor.o JitBuilder.o PrettyPrinter.o
	ar -cr $(LIBJITB2) \
		Action.o \
		Builder.o \
		BuilderBase.o \
		CodeGenerator.o \
		DialectReducer.o \
		FunctionBuilder.o \
		Location.o \
		Object.o \
		Operation.o \
		OperationBase.o \
		PrettyPrinter.o \
		Transformer.o \
		Type.o \
		TypeDictionary.o \
		TypeGraph.o \
		Value.o \
		Visitor.o \
		JitBuilder.o

#CXXFLAGS=-O3 -g -std=c++0x -fno-rtti -fPIC -Wwritable-strings
CXXFLAGS=-O0 -g -std=c++0x -fno-rtti -fPIC -Wwritable-strings

.cpp.o:
	g++ $(CXXFLAGS) -c $<

Action.o: Action.cpp Action.hpp
Builder.o: Builder.cpp Builder.hpp Operation.hpp Type.hpp Value.hpp
DialectReducer.o: DialectReducer.cpp DialectReducer.hpp
FunctionBuilder.o: FunctionBuilder.cpp FunctionBuilder.hpp
Location.o: Location.cpp Location.hpp
Operation.o: Operation.cpp Operation.hpp
PrettyPrinter.o: PrettyPrinter.cpp PrettyPrinter.hpp Builder.hpp
Type.o: Type.cpp Type.hpp
TypeDictionary.o: TypeDictionary.cpp TypeDictionary.hpp
TypeGraph.o: TypeGraph.cpp TypeGraph.hpp PrettyPrinter.hpp
Value.o: Value.cpp Value.hpp
Visitor.o: Visitor.cpp Visitor.hpp

JitBuilder.o: JitBuilder.cpp JitBuilder.hpp PrettyPrinter.hpp
	g++ $(CXXFLAGS) -c -I. -I${OMRDIR}/jitbuilder/x/amd64 -I${OMRDIR}/jitbuilder/x -I${OMRDIR}/jitbuilder -I${OMRDIR}/compiler/x/amd64 -I${OMRDIR}/compiler/x -I${OMRDIR}/compiler -I${OMRDIR}/ -I${OMRDIR}/include_core -I${OMRDIR}/build.osx $<

CodeGenerator.o: CodeGenerator.cpp CodeGenerator.hpp
	g++ $(CXXFLAGS) -c -I. -I${OMRDIR}/jitbuilder/x/amd64 -I${OMRDIR}/jitbuilder/x -I${OMRDIR}/jitbuilder -I${OMRDIR}/compiler/x/amd64 -I${OMRDIR}/compiler/x -I${OMRDIR}/compiler -I${OMRDIR}/ -I${OMRDIR}/include_core -I${OMRDIR}/build.osx $<

testTypeGraph.o: testTypeGraph.cpp Action.hpp TypeDictionary.hpp TypeGraph.hpp

clean:
	rm -f matmult *.o $(LIBJITB2)
	cd ComplexMatMult; rm -f cmatmult *.o
