

LIBDNG     = libdng_sdk.a

# The XMP SDK build directory if we don't want to install it system-wide.
XMP_PUB_DIR= ../../../../xmp_sdk/public

XMP_RELEASE= $(XMP_PUB_DIR)/libraries/i80386linux_x64/release/

SRC_DIR = ../../source

CXXFLAGS  := -pipe -I$(XMP_PUB_DIR)/include -I$(SRC_DIR) -I$(CURDIR) -O0 -ggdb
LDFLAGS   := -ljpeg -lz -lpthread -ldl $(XMP_RELEASE)/staticXMPFiles.ar $(XMP_RELEASE)/staticXMPCore.ar -ldl
ARFLAGS   := crus
MAKEFLAGS := --no-print-directory

VPATH = $(SRC_DIR)

LIB_CPP_FILES := dng_1d_function.cpp \
	dng_camera_profile.cpp \
	dng_file_stream.cpp \
	dng_hue_sat_map.cpp \
	dng_jpeg_image.cpp \
	dng_memory_stream.cpp \
	dng_opcodes.cpp \
	dng_pthread.cpp \
	dng_render.cpp \
	dng_string.cpp \
	dng_utils.cpp \
	dng_1d_table.cpp \
	dng_color_space.cpp \
	dng_filter_task.cpp \
	dng_ifd.cpp \
	dng_lens_correction.cpp \
	dng_misc_opcodes.cpp \
	dng_orientation.cpp \
	dng_rational.cpp \
	dng_resample.cpp \
	dng_string_list.cpp \
	dng_abort_sniffer.cpp \
	dng_color_spec.cpp \
	dng_fingerprint.cpp \
	dng_image.cpp \
	dng_linearization_info.cpp \
	dng_mosaic_info.cpp \
	dng_parse_utils.cpp \
	dng_read_image.cpp \
	dng_shared.cpp \
	dng_tag_types.cpp \
	dng_xmp.cpp \
	dng_area_task.cpp \
	dng_date_time.cpp \
	dng_gain_map.cpp \
	dng_image_writer.cpp \
	dng_lossless_jpeg.cpp \
	dng_mutex.cpp \
	dng_pixel_buffer.cpp \
	dng_rect.cpp \
	dng_simple_image.cpp \
	dng_temperature.cpp \
	dng_xmp_sdk.cpp \
	dng_bad_pixels.cpp \
	dng_exceptions.cpp \
	dng_globals.cpp \
	dng_info.cpp \
	dng_matrix.cpp \
	dng_negative.cpp \
	dng_point.cpp \
	dng_ref_counted_block.cpp \
	dng_spline.cpp \
	dng_tile_iterator.cpp \
	dng_xy_coord.cpp \
	dng_bottlenecks.cpp \
	dng_exif.cpp \
	dng_host.cpp \
	dng_iptc.cpp \
	dng_memory.cpp \
	dng_opcode_list.cpp \
	dng_preview.cpp \
	dng_reference.cpp \
	dng_stream.cpp \
	dng_tone_curve.cpp

LIB_OBJ_FILES := $(LIB_CPP_FILES:.cpp=.o)

.PHONY: all clean

# Execute the binary
all: dng_validate $(LIBDNG)

# Linking
dng_validate: dng_validate.o $(LIBDNG)
	$(CXX) $^ $(LDFLAGS) -o $@

$(LIBDNG): $(LIB_OBJ_FILES)
	$(AR) $(ARFLAGS) $@ $^

dng_validate.o: $(SRC_DIR)/dng_validate.cpp
	$(CXX) $(CXXFLAGS) -DqDNGValidateTarget -c $^

clean:
	-rm -f dng_validate *.o $(LIBDNG)
