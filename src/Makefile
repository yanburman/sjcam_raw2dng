# Binary name
APP=sjcam_raw2dng

# The XMP SDK build directory if we don't want to install it system-wide.
XMP_PUB_DIR=../xmp_sdk/public
DNG_PUB_DIR=../dng_sdk

INCL=-I$(XMP_PUB_DIR)/include -I$(DNG_PUB_DIR)/source -I$(DNG_PUB_DIR)/projects/linux
XMP_RELEASE=$(XMP_PUB_DIR)/libraries/i80386linux_x64/release/
DNG_BIN_DIR=$(DNG_PUB_DIR)/projects/linux
LIB= $(DNG_BIN_DIR)/libdng_sdk.a -ljpeg -lz -lpthread -ldl $(XMP_RELEASE)/staticXMPFiles.ar $(XMP_RELEASE)/staticXMPCore.ar -ldl

CPP_FILES := sjcam_raw2dng.cpp \
	CFAReader.cpp \
	DNGConverter.cpp
OBJ_FILES := $(CPP_FILES:.cpp=.o)


# Execute the binary
all: $(APP)

# Linking
$(APP): $(OBJ_FILES)
	g++ $(OBJ_FILES) $(LIB) -o $(APP)

# Compilation
%.o: %.cpp
	-@clang-format-3.6 -i $^
	g++ -c -O0 -ggdb -Wall $(INCL) $^

clean:
	-rm $(APP) *.o
