LIBRETRO_INC=libretro-common/include

BUILD_DIR=build

all: libretro_volcano.so

run: all
	retroarch -L libretro_volcano.so

libretro-common/vulkan/vulkan_symbol_wrapper.c:
$(LIBRETRO_INC):
	git submodule update

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/%.o: %.cpp $(BUILD_DIR) $(LIBRETRO_INC)
	g++ -c $< -I$(LIBRETRO_INC) -o $@ -fPIC

$(BUILD_DIR)/vulkan_symbol_wrapper.o: libretro-common/vulkan/vulkan_symbol_wrapper.c $(BUILD_DIR) $(LIBRETRO_INC)
	gcc -c $< -o $@ -I$(LIBRETRO_INC) -fPIC

libretro_volcano.so: $(BUILD_DIR)/libvolcano.o $(BUILD_DIR)/vulkan_symbol_wrapper.o
	g++ -o $@ $^ -shared
