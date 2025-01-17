###############################################################################
# Copyright (c) 2022, 2022 IBM Corp. and others
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

omr_assert(
	TEST OMR_JB2
	MESSAGE "OMR_JB2 must be set."
)

set(GTEST_DIR $(omr_SOURCE_DIR)/third_party/gtest-1.8.0)

#if(OMR_JB2_TEST)
	add_executable(testcomplex
		ComplexMatMult.cpp
	)
	target_link_libraries(testcomplex
		jb2complex
		jb2base
		jb2
		omrGtest
		${CMAKE_DL_LIBS}
	)
	target_include_directories(testcomplex PRIVATE
		${JB2_CORE_DIR}
	       	${GTEST_DIR}/include
	)

	add_test(
		NAME testcomplex
		COMMAND $<TARGET_FILE:testcomplex>
	)

#endif()
