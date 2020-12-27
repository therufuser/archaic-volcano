LIBRETRO_INC=libretro-common/include

BUILD_DIR=build

all: libretro_volcano.so

run: all
	retroarch -L libretro_volcano.so

$(LIBRETRO_INC):
	git submodule update

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/%.o: %.cpp $(BUILD_DIR) $(LIBRETRO_INC)
	g++ -c $< -I$(LIBRETRO_INC) -o $@

libretro_volcano.so: $(BUILD_DIR)/libvolcano.o
	g++ -o $@ $< -shared
