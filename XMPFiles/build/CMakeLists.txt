# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

# ==============================================================================
# define minimum cmake version
cmake_minimum_required(VERSION 2.8.6)

# Enable folder grouping of projects in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ==============================================================================
# This project
set(TARGET_NAME XMPFiles)
if(CMAKE_CL_64)
	project(${TARGET_NAME}64)
else(CMAKE_CL_64)
	project(${TARGET_NAME})
endif(CMAKE_CL_64)

set(STATIC_STR "Static")
if (XMP_BUILD_STATIC)
	set(TARGET_NAME "${TARGET_NAME}${STATIC_STR}")
	set(PROJECT_LABEL_STR "${TARGET_NAME}")
else(XMP_BUILD_STATIC)
	set(PROJECT_LABEL_STR "${TARGET_NAME}")
endif(XMP_BUILD_STATIC)

# ==============================================================================
# Shared config
# ==============================================================================
# setup some values before calling shared config
set(XMP_THIS_PROJECT_RELATIVEPATH "../..")

include(${CMAKE_CURRENT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/build/XMP_Config.cmake)


set(TP_ZUID_PATH "${XMPROOT_DIR}/third-party/zuid/interfaces")
set(LIB_ADOBEXMP XMPCore)

# ==============================================================================
# platform specific config
# ==============================================================================
if(UNIX)
	if(APPLE)
		# OSX -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "XMPFilesStatic")
		else()
			set(XMPFILES_LIB "XMPFiles")
		endif()
	else(APPLE)
		# Linux -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "staticXMPFiles")
		else()
			set(XMPFILES_LIB "XMPFiles")
		endif()
	endif(APPLE)
else(UNIX)
	if(WIN32)
		# Windows -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "XMPFilesStatic")
		else(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "XMPFiles")
		endif()
	endif(WIN32)
endif(UNIX)


include(${CMAKE_CURRENT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/XMPFiles/build/CMakeListsCommon.txt)