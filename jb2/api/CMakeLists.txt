###############################################################################
# Copyright (c) 2017, 2021 IBM Corp. and others
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

omr_assert(
	TEST OMR_JB2
	MESSAGE "OMR_JB2 must be set."
)

#target_link_libraries(jb2
#	PUBLIC
#		${OMR_PORT_LIB}
#)

if (NOT APPLE)
    add_compile_options(
	    -Wplacement-new=0
    )
endif()

add_compile_options(
    -I${CMAKE_BINARY_DIR}
)

if (APPLE)
    add_link_options(
        -undefined dynamic_lookup
    )
endif()

set(JB2_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/)
set(JB2_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Base/)
set(JB2_DEBUG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Debug/)
set(JB2_VM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vm/)
set(JB2_COMPLEX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Complex/)
set(JB2_JB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jb)

add_library(jb2core SHARED
    Addon.cpp
    AllocationCategoryClasses.cpp
    AllocationCategoryService.cpp
    Allocator.cpp
    AllocatorRaw.cpp
    AllocatorTracer.cpp
    AllocatorTracker.cpp
    Builder.cpp
    BuilderEntry.cpp
    CodeGenerator.cpp
    Compilation.cpp
    CompiledBody.cpp
    Compiler.cpp
    CompileUnit.cpp
    Config.cpp
    Context.cpp
    CoreExtension.cpp
    CreateLoc.cpp
    EntryPoint.cpp
    Extensible.cpp
    Extension.cpp
    InputReader.cpp
    IR.cpp
    IRCloner.cpp
    KindService.cpp
    Literal.cpp
    LiteralDictionary.cpp
    Location.cpp
    Loggable.cpp
    NativeEntry.cpp
    Operation.cpp
    OperationCloner.cpp
    OperationReplacer.cpp
    Pass.cpp
    Scope.cpp
    SemanticVersion.cpp
    Strategy.cpp
    String.cpp
    Symbol.cpp
    SymbolDictionary.cpp
    TextLogger.cpp
    TextWriter.cpp
    Transformer.cpp
    Type.cpp
    TypeDictionary.cpp
    TypeReplacer.cpp
    Value.cpp
    Visitor.cpp
)

#set(VALGRIND "valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all ")
set(VALGRIND "")

add_subdirectory(test)

# Extensions
add_subdirectory(Func)
add_subdirectory(Base)
add_subdirectory(jb)
add_subdirectory(vm)
#add_subdirectory(Debug)
#add_subdirectory(Sim)
#add_subdirectory(Complex)
