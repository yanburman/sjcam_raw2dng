
cmake_minimum_required(VERSION 2.8.6)

# Enable folder grouping of projects in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "No build type selected, defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Our project
project(sjcam_raw2dng)

# import XMP SDK
set(XMPROOT "xmp_sdk")

# import DNG SDK
add_subdirectory(dng_sdk/projects/cmake)


set(SRC_DIR src)

if (UNIX)
    if (APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAC_ENV=1")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack")
        set(warning_flags "-Wall \
                           -Wsign-compare \
                           -Wclobbered \
                           -Wempty-body \
                           -Wignored-qualifiers \
                           -Wsign-compare \
                           -Wtype-limits \
                           -Wunused-but-set-parameter \
                           -Wconversion \
                           -Winit-self \
                           -Wno-unused-function")

        set(CMAKE_CXX_FLAGS_DEBUG "${warning_flags} \
                                   -O0 \
                                   -ggdb \
                                   -D_FORTIFY_SOURCE=2")

        set(CMAKE_CXX_FLAGS_RELEASE "${warning_flags} \
                                     -O2 \
                                     -DNDEBUG \
                                     -U_FORTIFY_SOURCE")
    endif (APPLE)
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DqDNG64Bit /DqMacOS=0 /DqWinOS=1 /DqLinux=0 /D_CRT_SECURE_NO_DEPRECATE /DqDNGXMPDocOps=0")
endif(MSVC)

# cmake adds -rdynamic to all links which effectively disables lto
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")


set(target sjcam_raw2dng)

add_executable(${target} ${SRC_DIR}/CFAReader.cpp
                         ${SRC_DIR}/DNGConverter.cpp
                         ${SRC_DIR}/utils.cpp
                         ${SRC_DIR}/sjcam_raw2dng.cpp)

target_include_directories(${target} PUBLIC ${SRC_DIR}
                                            dng_sdk/source
                                            ${XMPROOT}/public/include)

if (UNIX)
    target_link_libraries(${target}
                          dng_sdk
                          -ljpeg
                          -lz
                          -lpthread
                          -ldl
                          XMPFilesStatic
                          XMPCoreStatic
                          -ldl
                          )
endif(UNIX)

if (MSVC)
    target_link_libraries(${target}
                          dng_sdk
                          XMPFilesStatic
                          XMPCoreStatic
                          )
endif(MSVC)
