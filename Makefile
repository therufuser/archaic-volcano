LIBRETRO_INC=libretro-common/include
LOGURU_INC=loguru

SHADER_PATH=volcano/shaders
SHADER_FILES=$(SHADER_PATH)/triangle.vert.inc $(SHADER_PATH)/triangle.frag.inc

BUILD_DIR=build

all: libretro_volcano.so

run: all
	retroarch -L libretro_volcano.so

$(LIBRETRO_INC) libretro-common/vulkan/vulkan_symbol_wrapper.c:
	git submodule update

$(LOGURU_INC) loguru/loguru.hpp:
	git submodule update

%/:
	mkdir -p $@

$(SHADER_PATH)/%.frag.inc: $(SHADER_PATH)/%.frag
	glslc -mfmt=c $< -o $@

$(SHADER_PATH)/%.vert.inc: $(SHADER_PATH)/%.vert
	glslc -mfmt=c $< -o $@

.SECONDEXPANSION:
$(BUILD_DIR)/%.o: %.cpp $(LIBRETRO_INC) $(LOGURU_INC) $(SHADER_FILES) | $$(@D)/
	g++ -c $< -I$(LIBRETRO_INC) -I$(LOGURU_INC) -o $@ -fPIC

$(BUILD_DIR)/vulkan_symbol_wrapper.o: libretro-common/vulkan/vulkan_symbol_wrapper.c $(LIBRETRO_INC) | $(BUILD_DIR)/
	gcc -c $< -o $@ -I$(LIBRETRO_INC) -fPIC

$(BUILD_DIR)/loguru.o: $(LOGURU_INC)/loguru.cpp $(LOGURU_INC) | $(BUILD_DIR)/
	gcc -c $< -o $@ -I$(LOGURU_INC) -fPIC

libretro_volcano.so: $(BUILD_DIR)/loguru.o $(BUILD_DIR)/libvolcano.o $(BUILD_DIR)/vulkan_symbol_wrapper.o $(BUILD_DIR)/volcano/volcano.o $(BUILD_DIR)/volcano/renderer/mesh.o
	g++ -o $@ $^ -shared
