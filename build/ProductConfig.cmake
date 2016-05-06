#/*************************************************************************
#*
#* ADOBE CONFIDENTIAL
#* ___________________
#*
#* Copyright [2012] Adobe Systems Incorporated
#* All Rights Reserved.
#*
#* NOTICE:  All information contained herein is, and remains
#* the property of Adobe Systems Incorporated and its suppliers,
#* if any.  The intellectual and technical concepts contained
#* herein are proprietary to Adobe Systems Incorporated and its
#* suppliers and are protected by trade secret or copyright law.
#* Dissemination of this information or reproduction of this material
#* is strictly forbidden unless prior written permission is obtained
#* from Adobe Systems Incorporated.
#**************************************************************************/

# ==============================================================================
# define minimum cmake version
cmake_minimum_required(VERSION 2.8.0)

# ==============================================================================
# Product Config for XMP Toolkit
# ==============================================================================
if (UNIX)
	if (APPLE)
		set(XMP_ENABLE_SECURE_SETTINGS "ON")
		
		if (APPLE_IOS)
			set(XMP_PLATFORM_SHORT "ios")
			set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_32_BIT)")
			add_definitions(-DIOS_ENV=1)			

			# shared flags
			set(XMP_SHARED_COMPILE_FLAGS "${XMP_SHARED_COMPILE_FLAGS}  ${XMP_EXTRA_COMPILE_FLAGS}")
			set(XMP_SHARED_COMPILE_DEBUG_FLAGS "-O0 -DDEBUG=1 -D_DEBUG=1")
			set(XMP_SHARED_COMPILE_RELEASE_FLAGS "-Os -DNDEBUG=1 -D_NDEBUG=1")

			set(XMP_PLATFORM_BEGIN_WHOLE_ARCHIVE "")
			set(XMP_PLATFORM_END_WHOLE_ARCHIVE "")
		else ()
			set(XMP_PLATFORM_SHORT "mac")
			if(CMAKE_CL_64)
				set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for OSX" FORCE)
				add_definitions(-DXMP_64=1)
			else(CMAKE_CL_64)
				set(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
				add_definitions(-DXMP_64=0)
			endif(CMAKE_CL_64)

			# is SDK and deployment target set?
			if(NOT DEFINED XMP_OSX_SDK)
				# no, so default to CS6 settings
				#set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "4.2")
				set(XMP_OSX_SDK		10.8)
				set(XMP_OSX_TARGET	10.7)
			endif()

			add_definitions(-DMAC_ENV=1)


			#
			# shared flags
			#
			set(XMP_SHARED_COMPILE_FLAGS "-Wall -Wextra")
			set(XMP_SHARED_COMPILE_FLAGS "${XMP_SHARED_COMPILE_FLAGS} -Wno-missing-field-initializers -Wno-shadow") # disable some warnings

			set(XMP_SHARED_COMPILE_FLAGS "${XMP_SHARED_COMPILE_FLAGS}")

			set(XMP_SHARED_CXX_COMPILE_FLAGS "${XMP_SHARED_COMPILE_FLAGS} -Wno-reorder") # disable warnings

			set(XMP_PLATFORM_BEGIN_WHOLE_ARCHIVE "")
			set(XMP_PLATFORM_END_WHOLE_ARCHIVE "")
			set(XMP_DYLIBEXTENSION	"dylib")
		endif ()

		#There were getting set from SetupTargetArchitecture. 
		if(APPLE_IOS)
			set(XMP_CPU_FOLDERNAME	"$(ARCHS_STANDARD_32_BIT)")
		else()
			if(CMAKE_CL_64)
				set(XMP_BITDEPTH		"64")
				set(XMP_CPU_FOLDERNAME	"intel_64")
			else()
				set(XMP_BITDEPTH		"32")
				set(XMP_CPU_FOLDERNAME	"intel")
			endif()
		endif()

		# XMP_PLATFORM_FOLDER is used in OUTPUT_DIR and Debug/Release get automatically added for VS/XCode projects
		if(APPLE_IOS)
		    set(XMP_PLATFORM_FOLDER "ios/${XMP_CPU_FOLDERNAME}")
		else()
		    set(XMP_PLATFORM_FOLDER "macintosh/${XMP_CPU_FOLDERNAME}")
		endif()
	else ()
		if(NOT CMAKE_CROSSCOMPILING)
			if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
				# running on 64bit machine
				set(XMP_EXTRA_BUILDMACHINE	"Buildmachine is 64bit")
			else()
				# running on 32bit machine
				set(XMP_EXTRA_BUILDMACHINE	"Buildmachine is 32bit")
			endif()

			if(CMAKE_CL_64)
				set(XMP_EXTRA_COMPILE_FLAGS "-m64")
				set(XMP_EXTRA_LINK_FLAGS "-m64")
				set(XMP_PLATFORM_FOLDER "i80386linux_x64") # add XMP_BUILDMODE_DIR to follow what other platforms do
				set(XMP_GCC_LIBPATH /user/unicore/i80386linux_x64/compiler/gcc4.4.4/linux2.6_64/lib64)
			else()
				set(XMP_EXTRA_LINK_FLAGS "-m32 -mtune=i686")
				set(XMP_PLATFORM_FOLDER "i80386linux") # add XMP_BUILDMODE_DIR to follow what other platforms do
				set(XMP_GCC_LIBPATH /user/unicore/i80386linux/compiler/gcc4.4.4/linux2.6_32/lib)
			endif()
		else()
			# running toolchain
			if(CMAKE_CL_64)
				set(XMP_EXTRA_COMPILE_FLAGS "-m64")
				set(XMP_EXTRA_LINK_FLAGS "-m64")
				set(XMP_PLATFORM_FOLDER "i80386linux_x64") # add XMP_BUILDMODE_DIR to follow what other platforms do
				set(XMP_GCC_LIBPATH /user/unicore/i80386linux_x64/compiler/gcc4.4.4/linux2.6_64/lib64)
			else()
				set(XMP_EXTRA_COMPILE_FLAGS "-m32 -mtune=i686")
				set(XMP_EXTRA_LINK_FLAGS "-m32")
				set(XMP_PLATFORM_FOLDER "i80386linux") # add XMP_BUILDMODE_DIR to follow what other platforms do
				set(XMP_GCC_LIBPATH /user/unicore/i80386linux/compiler/gcc4.4.4/linux2.6_32/lib)
			endif()

			set(XMP_EXTRA_BUILDMACHINE	"Cross compiling")
		endif()
		set(XMP_PLATFORM_VERSION "linux2.6") # add XMP_BUILDMODE_DIR to follow what other platforms do

		add_definitions(-DUNIX_ENV=1)
		# Linux -------------------------------------------
		set(XMP_PLATFORM_SHORT "linux")
		#gcc path is not set correctly
		#set(CMAKE_C_COMPILER "/user/unicore/i80386linux/compiler/gcc4.4.4/linux2.6_32/bin/gcc")
		#set(CMAKE_CXX_COMPILER "/user/unicore/i80386linux/compiler/gcc4.4.4/linux2.6_32/bin/gcc")
		#set(XMP_GCC_LIBPATH /user/unicore/i80386linux/compiler/gcc4.4.4/linux2.6_32/lib)
		set(XMP_PLATFORM_LINK "-z defs -Xlinker -Bsymbolic -Wl,--no-undefined ${XMP_EXTRA_LINK_FLAGS} ${XMP_TOOLCHAIN_LINK_FLAGS} -lrt -ldl -luuid -lpthread ${XMP_GCC_LIBPATH}/libssp.a")
		set(XMP_SHARED_COMPILE_FLAGS "-Wno-multichar -Wno-implicit -D_FILE_OFFSET_BITS=64 -funsigned-char  ${XMP_EXTRA_COMPILE_FLAGS} ${XMP_TOOLCHAIN_COMPILE_FLAGS}")
		set(XMP_SHARED_COMPILE_DEBUG_FLAGS " ")
		set(XMP_SHARED_COMPILE_RELEASE_FLAGS "-fwrapv ")

		if(NOT XMP_DISABLE_FASTMATH)
		#	set(XMP_SHARED_COMPILE_FLAGS "${XMP_SHARED_COMPILE_FLAGS} -ffast-math")
		#	set(XMP_SHARED_COMPILE_RELEASE_FLAGS "${XMP_SHARED_COMPILE_RELEASE_FLAGS} -fomit-frame-pointer -funroll-loops")
		endif()
	endif()
else ()
	if(CMAKE_CL_64)
		set(XMP_PLATFORM_FOLDER "windows_x64") # leave XMP_BUILDMODE_DIR away, since CMAKE_CFG_INTDIR gets added by CMake automatically
	else(CMAKE_CL_64)
		set(XMP_PLATFORM_FOLDER "windows") # leave XMP_BUILDMODE_DIR away, since CMAKE_CFG_INTDIR gets added by CMake automatically
	endif(CMAKE_CL_64)
	set(XMP_PLATFORM_SHORT "win")
	set(XMP_PLATFORM_LINK "")
	set(XMP_SHARED_COMPILE_FLAGS "-DWIN_ENV=1 -D_CRT_SECURE_NO_WARNINGS=1 -D_SCL_SECURE_NO_WARNINGS=1  /J /fp:precise")
	set(XMP_SHARED_COMPILE_DEBUG_FLAGS "")
	set(XMP_SHARED_COMPILE_RELEASE_FLAGS "/O1 /Ob2 /Os /Oy- /GL /FD ")

	set(XMP_SHARED_COMPILE_DEBUG_FLAGS "${XMP_SHARED_COMPILE_DEBUG_FLAGS} /MDd")
	set(XMP_SHARED_COMPILE_RELEASE_FLAGS "${XMP_SHARED_COMPILE_RELEASE_FLAGS} /MD")

	set(XMP_PLATFORM_LINK_WIN "${XMP_WIN32_LINK_EXTRAFLAGS} /MAP")
endif ()


