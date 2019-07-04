###############################################################################
# Copyright (c) 2017, 2018 IBM Corp. and others
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution and
# is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following
# Secondary Licenses when the conditions for such availability set
# forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
# General Public License, version 2 with the GNU Classpath
# Exception [1] and GNU General Public License, version 2 with the
# OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
###############################################################################

include(OmrCompilerSupport)

find_package(PythonInterp 3.1 REQUIRED)

# JitBuilder Files
set(JITBUILDER_OBJECTS
	env/FrontEnd.cpp
	compile/Method.cpp
	control/Jit.cpp
	ilgen/JBIlGeneratorMethodDetails.cpp
	optimizer/JBOptimizer.hpp
	optimizer/JBOptimizer.cpp
	optimizer/Optimizer.hpp
	runtime/JBCodeCacheManager.cpp
)

if(OMR_ARCH_X86)
	list(APPEND JITBUILDER_OBJECTS
		x/codegen/Evaluator.cpp
	)
elseif(OMR_ARCH_S390)
	list(APPEND JITBUILDER_OBJECTS
		z/codegen/Evaluator.cpp
		z/codegen/JBCodeGenerator.cpp
	)
elseif(OMR_ARCH_POWER)
	list(APPEND JITBUILDER_OBJECTS
		p/codegen/Evaluator.cpp
		p/runtime/CodeSync.cpp
		p/runtime/AsmUtil.spp
		p/runtime/CodeDispatch.spp
		${omr_SOURCE_DIR}/compiler/p/env/OMRDebugEnv.cpp
	)
endif()

set(JITBUILDER_FILES
	BytecodeBuilder
	IlBuilder
	IlType
	IlValue
	MethodBuilder
	ThunkBuilder
	TypeDictionary
	VirtualMachineOperandArray
	VirtualMachineOperandStack
	VirtualMachineRegister
	VirtualMachineRegisterInStruct
	VirtualMachineState
	JitBuilder
)

# Generated client API files
set(JITBUILDER_API_DESCRIPTION ${CMAKE_CURRENT_SOURCE_DIR}/apigen/jitbuilder.api.json)

# CPP client
if (OMR_JITBUILDER_BINDING_CPP)
	set(CPP_API_GENERATOR ${CMAKE_CURRENT_SOURCE_DIR}/apigen/cppgen.py)
	set(JITBUILDER_CPP_API_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release/cpp/include)
	set(JITBUILDER_CPP_API_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}client/cpp)
	set(JITBUILDER_CPP_API_SOURCES
		${JITBUILDER_CPP_API_SOURCE_DIR}/BytecodeBuilder.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/IlBuilder.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/IlType.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/IlValue.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/MethodBuilder.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/ThunkBuilder.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/TypeDictionary.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/VirtualMachineOperandArray.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/VirtualMachineOperandStack.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/VirtualMachineRegister.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/VirtualMachineRegisterInStruct.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/VirtualMachineState.cpp
		${JITBUILDER_CPP_API_SOURCE_DIR}/JitBuilder.cpp
	)

	set(JITBUILDER_CPP_API_HEADERS
		${JITBUILDER_CPP_API_HEADER_DIR}/BytecodeBuilder.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/IlBuilder.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/IlType.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/IlValue.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/MethodBuilder.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/ThunkBuilder.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/TypeDictionary.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/VirtualMachineOperandArray.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/VirtualMachineOperandStack.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/VirtualMachineRegister.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/VirtualMachineRegisterInStruct.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/VirtualMachineState.hpp
		${JITBUILDER_CPP_API_HEADER_DIR}/JitBuilder.hpp
	)
	
	add_custom_command(
		OUTPUT ${JITBUILDER_CPP_API_SOURCES} ${JITBUILDER_CPP_API_HEADERS}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${JITBUILDER_CPP_API_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${JITBUILDER_CPP_API_HEADER_DIR}
		COMMAND ${PYTHON_EXECUTABLE} ${CPP_API_GENERATOR} ${JITBUILDER_API_DESCRIPTION} --sourcedir ${JITBUILDER_CPP_API_SOURCE_DIR} --headerdir ${JITBUILDER_CPP_API_HEADER_DIR}
		DEPENDS ${CPP_API_GENERATOR} ${JITBUILDER_API_DESCRIPTION}
		COMMENT "Running JitBuilder C++ API generator"
	)

	list(APPEND JITBUILDER_OBJECTS
		${JITBUILDER_CPP_API_SOURCES}
	)
endif()

# C client
if (OMR_JITBUILDER_BINDING_C)
	set(C_API_GENERATOR ${CMAKE_CURRENT_SOURCE_DIR}/apigen/cgen.py)
	set(JITBUILDER_C_API_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release/c/include)
	set(JITBUILDER_C_API_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}client/c)

	set(JITBUILDER_C_API_SOURCES
		${JITBUILDER_C_API_SOURCE_DIR}/BytecodeBuilder.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/IlBuilder.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/IlType.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/IlValue.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/MethodBuilder.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/ThunkBuilder.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/TypeDictionary.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/VirtualMachineOperandArray.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/VirtualMachineOperandStack.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/VirtualMachineRegister.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/VirtualMachineRegisterInStruct.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/VirtualMachineState.cpp
		${JITBUILDER_C_API_SOURCE_DIR}/JitBuilder.cpp
	)

	set(JITBUILDER_C_API_HEADERS
		${JITBUILDER_C_API_HEADER_DIR}/BytecodeBuilder.h
		${JITBUILDER_C_API_HEADER_DIR}/IlBuilder.h
		${JITBUILDER_C_API_HEADER_DIR}/IlType.h
		${JITBUILDER_C_API_HEADER_DIR}/IlValue.h
		${JITBUILDER_C_API_HEADER_DIR}/MethodBuilder.h
		${JITBUILDER_C_API_HEADER_DIR}/ThunkBuilder.h
		${JITBUILDER_C_API_HEADER_DIR}/TypeDictionary.h
		${JITBUILDER_C_API_HEADER_DIR}/VirtualMachineOperandArray.h
		${JITBUILDER_C_API_HEADER_DIR}/VirtualMachineOperandStack.h
		${JITBUILDER_C_API_HEADER_DIR}/VirtualMachineRegister.h
		${JITBUILDER_C_API_HEADER_DIR}/VirtualMachineRegisterInStruct.h
		${JITBUILDER_C_API_HEADER_DIR}/VirtualMachineState.h
		${JITBUILDER_C_API_HEADER_DIR}/JitBuilder.h
	)

	add_custom_command(
		OUTPUT ${JITBUILDER_C_API_SOURCES} ${JITBUILDER_C_API_HEADERS}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${JITBUILDER_C_API_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${JITBUILDER_C_API_HEADER_DIR}
		COMMAND ${PYTHON_EXECUTABLE} ${C_API_GENERATOR} ${JITBUILDER_API_DESCRIPTION} --sourcedir ${JITBUILDER_C_API_SOURCE_DIR} --headerdir ${JITBUILDER_C_API_HEADER_DIR}
		DEPENDS ${C_API_GENERATOR} ${JITBUILDER_API_DESCRIPTION}
		COMMENT "Running JitBuilder C API generator"
	)

	list(APPEND JITBUILDER_OBJECTS
		${JITBUILDER_C_API_SOURCES}
	)
endif()

# Java client
if (OMR_JITBUILDER_BINDING_JAVA)
	set(JAVA_API_GENERATOR ${CMAKE_CURRENT_SOURCE_DIR}/apigen/javagen.py)
	set(JITBUILDER_JAVA_JNI_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}client/java)
	set(JITBUILDER_JAVA_API_SOURCE_DIR ${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org/eclipse/omr/jitbuilder)

	set(JITBUILDER_JAVA_API_SOURCES
		${JITBUILDER_JAVA_API_SOURCE_DIR}/BytecodeBuilder.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/IlBuilder.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/IlType.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/IlValue.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/MethodBuilder.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/ThunkBuilder.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/TypeDictionary.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/VirtualMachineOperandArray.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/VirtualMachineOperandStack.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/VirtualMachineRegister.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/VirtualMachineRegisterInStruct.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/VirtualMachineState.java
		${JITBUILDER_JAVA_API_SOURCE_DIR}/JitBuilder.java
	)

	set(JITBUILDER_JAVA_JNI_SOURCES
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/BytecodeBuilder.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/IlBuilder.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/IlType.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/IlValue.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/MethodBuilder.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/ThunkBuilder.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/TypeDictionary.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/VirtualMachineOperandArray.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/VirtualMachineOperandStack.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/VirtualMachineRegister.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/VirtualMachineRegisterInStruct.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/VirtualMachineState.cpp
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/JitBuilder.cpp
	)

	set(JITBUILDER_JAVA_JNI_HEADERS
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_BytecodeBuilder.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_IlBuilder.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_IlType.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_IlValue.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_MethodBuilder.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_ThunkBuilder.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_TypeDictionary.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_VirtualMachineOperandArray.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_VirtualMachineOperandStack.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_VirtualMachineRegister.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_VirtualMachineRegisterInStruct.h
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}/org_eclipse_omr_jitbuilder_VirtualMachineState.h
	)

	add_custom_command(
		OUTPUT ${JITBUILDER_JAVA_API_SOURCES} ${JITBUILDER_JAVA_JNI_SOURCES}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${JITBUILDER_JAVA_JNI_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${JITBUILDER_JAVA_API_SOURCE_DIR}
		COMMAND ${PYTHON_EXECUTABLE} ${JAVA_API_GENERATOR} ${JITBUILDER_API_DESCRIPTION} --jnidir ${JITBUILDER_JAVA_JNI_SOURCE_DIR} --javadir ${JITBUILDER_JAVA_API_SOURCE_DIR}
		DEPENDS ${JAVA_API_GENERATOR} ${JITBUILDER_API_DESCRIPTION}
		COMMENT "Running JitBuilder Java API generator"
	)

	# --add-exports only needed for JDK9+ !!
	add_custom_command(
		OUTPUT ${JITBUILDER_JAVA_JNI_HEADERS}
		COMMAND javac --add-exports java.base/jdk.internal.org.objectweb.asm=ALL-UNNAMED -h ${JITBUILDER_JAVA_JNI_SOURCE_DIR} ${JITBUILDER_JAVA_API_SOURCE_DIR}/*.java
		DEPENDS ${JITBUILDER_JAVA_API_SOURCES}
		COMMENT "Generating JitBuilder Java API JNI headers"
	)

	# Get jni.h onto the include path; prefer to only apply to JITBUILDER_JAVA_JNI_SOURCES
	include_directories($ENV{JAVA_HOME}/include)

	## TOTAL HACK
	include_directories($ENV{JAVA_HOME}/include/darwin)

	list(APPEND JITBUILDER_OBJECTS
		${JITBUILDER_JAVA_JNI_SOURCES}
		${JITBUILDER_JAVA_JNI_HEADERS}
		${JITBUILDER_JAVA_API_SOURCES}
	)

endif()

# Create jitbuilder library.
create_omr_compiler_library(
	NAME    jitbuilder
	OBJECTS ${JITBUILDER_OBJECTS}
	DEFINES PROD_WITH_ASSUMES JITTEST
)

# Add interface path so that include paths propagate.
# NOTE: `release` directory  isn't being automatically setup, so this
#       is adding the actual compiler dir, as opposed to the 'composed'
#       install directory.
target_include_directories(jitbuilder
	INTERFACE
		${JITBUILDER_CPP_API_HEADER_DIR}
	INTERFACE
		${JITBUILDER_C_API_HEADER_DIR}
	INTERFACE
		${JITBUILDER_JAVA_JNI_SOURCE_DIR}
)

target_link_libraries(jitbuilder
	PUBLIC
		${OMR_PORT_LIB}
)

## JitBuilder examples only work on 64 bit currently.
if(OMR_JITBUILDER AND OMR_ENV_DATA64)
	add_subdirectory(release)
endif()

if(OMR_JITBUILDER_TEST)
	add_test(
		NAME TestJitBuilderAPIGenerator
		COMMAND ${PYTHON_EXECUTABLE} run_tests.py
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/apigen
	)
endif()

# install(TARGETS jitbuilder
#         ARCHIVE       DESTINATION ${CMAKE_BINARY_DIR}/jitbuilder_release
#         PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/jitbuilder_release/include
#         )
#
#
# #Create release directory
# file(MAKE_DIRECTORY jitbuilder_release)
# file(COPY ${jitbuilder_library} DESTINATION jitbuilder_release)
# file(COPY ${CMAKE_SOURCE_DIR}/compiler/env   DESTINATION jitbuilder_release/include)
# file(COPY ${CMAKE_SOURCE_DIR}/compiler/il    DESTINATION jitbuilder_release/include)
# file(COPY ${CMAKE_SOURCE_DIR}/compiler/ilgen DESTINATION jitbuilder_release/include)
