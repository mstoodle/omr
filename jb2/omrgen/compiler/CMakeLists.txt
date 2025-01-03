###############################################################################
# Copyright IBM Corp. and others 2017
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
# [2] https://openjdk.org/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
###############################################################################

include(OmrCompilerSupport)

# Compiler Files
set(JB2OMRCOMPILER_OBJECTS
	OMRCodeGenerator.cpp
	OMRIlGen.cpp
	OMRJB.cpp
	control/Jit.cpp
	${omr_SOURCE_DIR}/compiler/control/SimpleJit.cpp
)

if(OMR_ARCH_POWER)
	list(APPEND JB2OMRCOMPILER_OBJECTS
		p/runtime/CodeSync.cpp
		p/runtime/AsmUtil.spp
		p/runtime/CodeDispatch.spp
		${omr_SOURCE_DIR}/compiler/p/env/OMRDebugEnv.cpp
	)
endif()

if(OMR_ARCH_X86 OR OMR_ARCH_S390)
	set(OMR_ENHANCED_WARNINGS OFF)
else()
	set(OMR_WARNINGS_AS_ERRORS OFF)
	set(OMR_ENHANCED_WARNINGS OFF)
endif()

# Workaround for deprecated sprintf
if(OMR_OS_OSX)
	set(OMR_WARNINGS_AS_ERRORS OFF)
endif()

# Create jitbuilder library with "minimal" optimizer to reduce library size
create_omr_compiler_library(
	NAME    jb2omrcompiler
	OBJECTS ${JB2OMRCOMPILER_OBJECTS}
	DEFINES PROD_WITH_ASSUMES JITTEST
	FILTER	${omr_SOURCE_DIR}/compiler/optimizer/OMRFullOptimizer.cpp
)

target_include_directories(jb2omrcompiler PRIVATE
	${JB2_CORE_DIR}
)

target_link_libraries(jb2omrcompiler PUBLIC
	${OMR_PORT_LIB}
)
