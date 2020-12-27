LIBRETRO_INC=libretro-common/include

BUILD_DIR=build

all: libretro_volcano.so

run: all
	retroarch -L libretro_volcano.so

$(LIBRETRO_INC) libretro-common/vulkan/vulkan_symbol_wrapper.c:
	git submodule update

%/:
	mkdir -p $@

.SECONDEXPANSION:
$(BUILD_DIR)/%.o: %.cpp $(LIBRETRO_INC) | $$(@D)/
	g++ -c $< -I$(LIBRETRO_INC) -o $@ -fPIC

$(BUILD_DIR)/vulkan_symbol_wrapper.o: libretro-common/vulkan/vulkan_symbol_wrapper.c $(LIBRETRO_INC) | $(BUILD_DIR)/
	gcc -c $< -o $@ -I$(LIBRETRO_INC) -fPIC

libretro_volcano.so: $(BUILD_DIR)/libvolcano.o $(BUILD_DIR)/vulkan_symbol_wrapper.o $(BUILD_DIR)/volcano/volcano.o
	g++ -o $@ $^ -shared
