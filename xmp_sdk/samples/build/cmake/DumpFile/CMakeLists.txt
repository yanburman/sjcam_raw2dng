# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

# define minimum cmake version
cmake_minimum_required(VERSION 2.8.6)

# ==============================================================================
# Adding Project Name
# ==============================================================================
project (DumpFile)

# ==============================================================================
file (GLOB SOURCE_FILES ${SAMPLE_SOURCE_ROOT}/common/DumpFile.cpp ${SAMPLE_SOURCE_ROOT}/dumpfile/main.cpp )
file (GLOB COMMON_FILES ${SAMPLE_SOURCE_ROOT}/common/*.cpp  )
list(REMOVE_ITEM COMMON_FILES ${SAMPLE_SOURCE_ROOT}/common/DumpFile.cpp )


include(CMakeListsCommon.txt)



