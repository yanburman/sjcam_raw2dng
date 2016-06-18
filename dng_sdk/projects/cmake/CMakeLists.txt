
cmake_minimum_required(VERSION 2.8.6)

# Enable folder grouping of projects in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "No build type selected, defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(dng_sdk)


if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNOMINMAX")
endif (MSVC)

# import XMP SDK
set(XMPROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../xmp_sdk")
set(XMP_BUILD_STATIC "On")
set(CMAKE_TOOLCHAIN_FILE "${XMPROOT}/build/shared/ToolchangGCC.cmake")

add_subdirectory(${XMPROOT}/build ${XMPROOT}/build)



set(SRC_DIR ../../source)


set(LIB_SRC
	${SRC_DIR}/dng_1d_function.cpp
	${SRC_DIR}/dng_camera_profile.cpp
	${SRC_DIR}/dng_file_stream.cpp
	${SRC_DIR}/dng_hue_sat_map.cpp
	${SRC_DIR}/dng_jpeg_image.cpp
	${SRC_DIR}/dng_memory_stream.cpp
	${SRC_DIR}/dng_opcodes.cpp
	${SRC_DIR}/dng_pthread.cpp
	${SRC_DIR}/dng_render.cpp
	${SRC_DIR}/dng_string.cpp
	${SRC_DIR}/dng_utils.cpp
	${SRC_DIR}/dng_1d_table.cpp
	${SRC_DIR}/dng_color_space.cpp
	${SRC_DIR}/dng_filter_task.cpp
	${SRC_DIR}/dng_ifd.cpp
	${SRC_DIR}/dng_lens_correction.cpp
	${SRC_DIR}/dng_misc_opcodes.cpp
	${SRC_DIR}/dng_orientation.cpp
	${SRC_DIR}/dng_rational.cpp
	${SRC_DIR}/dng_resample.cpp
	${SRC_DIR}/dng_string_list.cpp
	${SRC_DIR}/dng_abort_sniffer.cpp
	${SRC_DIR}/dng_color_spec.cpp
	${SRC_DIR}/dng_fingerprint.cpp
	${SRC_DIR}/dng_image.cpp
	${SRC_DIR}/dng_linearization_info.cpp
	${SRC_DIR}/dng_mosaic_info.cpp
	${SRC_DIR}/dng_parse_utils.cpp
	${SRC_DIR}/dng_read_image.cpp
	${SRC_DIR}/dng_shared.cpp
	${SRC_DIR}/dng_tag_types.cpp
	${SRC_DIR}/dng_xmp.cpp
	${SRC_DIR}/dng_area_task.cpp
	${SRC_DIR}/dng_date_time.cpp
	${SRC_DIR}/dng_gain_map.cpp
	${SRC_DIR}/dng_image_writer.cpp
	${SRC_DIR}/dng_lossless_jpeg.cpp
	${SRC_DIR}/dng_mutex.cpp
	${SRC_DIR}/dng_pixel_buffer.cpp
	${SRC_DIR}/dng_rect.cpp
	${SRC_DIR}/dng_simple_image.cpp
	${SRC_DIR}/dng_temperature.cpp
	${SRC_DIR}/dng_xmp_sdk.cpp
	${SRC_DIR}/dng_bad_pixels.cpp
	${SRC_DIR}/dng_exceptions.cpp
	${SRC_DIR}/dng_globals.cpp
	${SRC_DIR}/dng_info.cpp
	${SRC_DIR}/dng_matrix.cpp
	${SRC_DIR}/dng_negative.cpp
	${SRC_DIR}/dng_point.cpp
	${SRC_DIR}/dng_ref_counted_block.cpp
	${SRC_DIR}/dng_spline.cpp
	${SRC_DIR}/dng_tile_iterator.cpp
	${SRC_DIR}/dng_xy_coord.cpp
	${SRC_DIR}/dng_bottlenecks.cpp
	${SRC_DIR}/dng_exif.cpp
	${SRC_DIR}/dng_host.cpp
	${SRC_DIR}/dng_iptc.cpp
	${SRC_DIR}/dng_memory.cpp
	${SRC_DIR}/dng_opcode_list.cpp
	${SRC_DIR}/dng_preview.cpp
	${SRC_DIR}/dng_reference.cpp
	${SRC_DIR}/dng_stream.cpp
	${SRC_DIR}/dng_tone_curve.cpp
   )

if (UNIX)
    set(DNG_FLAGS "-DqDNG64Bit")

    if (APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DNG_FLAGS} -DqMacOS=1 -DqDNGXMPDocOps=0")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack")
        set(CMAKE_CXX_FLAGS_DEBUG "${DNG_FLAGS} \
                                   -O0 \
                                   -ggdb \
                                   -D_FORTIFY_SOURCE=2")

        set(CMAKE_CXX_FLAGS_RELEASE "${DNG_FLAGS} \
                                     -O2 \
                                     -DNDEBUG \
                                     -U_FORTIFY_SOURCE")
    endif (APPLE)
else (UNIX)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DqDNG64Bit /DqMacOS=0 /DqWinOS=1 /DqLinux=0 /D_CRT_SECURE_NO_DEPRECATE /DqDNGXMPDocOps=0")
    endif (MSVC)
endif(UNIX)


# cmake adds -rdynamic to all links which effectively disables lto
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

include_directories(${SRC_DIR} ${XMPROOT}/public/include ${XMPROOT}/third-party/zlib)

# dng_validate binary
set(target dng_validate)

add_executable(${target}
		${LIB_SRC}
		${SRC_DIR}/dng_validate.cpp)

if (UNIX)
    target_compile_options(${target} PUBLIC
                           "-DqDNGValidateTarget=1")
else (UNIX)
    if (MSVC)
        target_compile_options(${target} PUBLIC
                               "/DqDNGValidateTarget=1")
    endif (MSVC)
endif (UNIX)


target_link_libraries(${target}
                                -ljpeg
                                -lz
                                -lpthread
                                -ldl
                                XMPFilesStatic
                                XMPCoreStatic
                                -ldl
                                )

# dng_sdk library
set(target dng_sdk)

add_library(${target} STATIC ${LIB_SRC})

