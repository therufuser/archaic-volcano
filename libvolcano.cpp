#include <libretro_vulkan.h>

#include <loguru.hpp>

#include <cmath>
#include <cstdio>
#include <vector>

#include "volcano/volcano.hpp"
#include "volcano/input/dispatcher.hpp"
#include "volcano/graphics/vertex.hpp"
#include "volcano/renderer/mesh.hpp"

#define WIDTH 1280
#define HEIGHT 720

static const char* library_name = "Archaic Volcano";
static const char* library_version = "v0.0.1";
static const char* valid_extensions = "";

static retro_environment_t env_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

volcano::input::dispatcher dispatcher;

static retro_hw_render_interface_vulkan* vulkan;
volcano::renderer renderer;

// Core basics
RETRO_API unsigned int retro_api_version () {
  return RETRO_API_VERSION;
}

RETRO_API void retro_get_system_info(retro_system_info* info) {
  info->library_name     = library_name;
  info->library_version  = library_version;
  info->valid_extensions = valid_extensions;
  info->need_fullpath    = false;
  info->block_extract    = false;
}

RETRO_API void retro_set_environment(retro_environment_t cb) {
  env_cb = cb;

  bool no_rom = true;
  cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

RETRO_API void retro_init() {
  loguru::add_file("logs/volcano_debug.log", loguru::Append, loguru::Verbosity_MAX);
  loguru::g_stderr_verbosity = 1;
  LOG_F(INFO, "Logger initialized!");
}

RETRO_API void retro_deinit() {

}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) {
  video_cb = cb;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb) {
  audio_cb = cb;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
  audio_batch_cb = cb;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb) {
  input_poll_cb = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb) {
  input_state_cb = cb;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device) {

}

// Core runtime stuff
RETRO_CALLCONV void retro_context_reset() {
  if(!env_cb(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, (void**)&vulkan) || !vulkan) {
    fprintf(stderr, "Could not fetch HW-render interface from frontend!");
    return;
  }

  fprintf(
    stderr, "Successfully fetched HW-interface: vulkan:\n\tinterface_version: %d\n\thandle: %p\n",
    vulkan->interface_version, vulkan->handle
  );

  renderer.init(vulkan);

  // Create a simple colored triangle
  static const float data[] = {
    -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // vec4 position, vec4 color
    -0.5f, +0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    +0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  };
//  renderer.add_mesh(data, sizeof(data) / sizeof(float));

  // Some other mesh
  const float circle_radius = 0.75f;
  const int circle_segments = 64;
  std::vector<volcano::graphics::vertex> circle_data;
  for(int i = 0; i < circle_segments; i++) {
    // Center vertex
    circle_data.push_back({
      .x = 0.0f,
      .y = 0.0f,
      .z = 0.0f,
      .w = 1.0f,

      .r = 0.5f,
      .g = 0.0f,
      .b = 0.0f,
      .a = 1.0f
    });

    // Diameter vertices (first)
    circle_data.push_back({
      .x = sin(i * 2 * M_PI / circle_segments) * circle_radius,
      .y = cos(i * 2 * M_PI / circle_segments) * circle_radius,
      .z = 0.03f * (i - (circle_segments >> 1)),
      .w = 1.0f,

      .r = 0.5f,
      .g = 0.0f,
      .b = 0.1f * i,
      .a = 1.0f
    });

    // Position data (second)
    circle_data.push_back({
      .x = sin((i + 1) * 2 * M_PI / circle_segments) * circle_radius,
      .y = cos((i + 1) * 2 * M_PI / circle_segments) * circle_radius,
      .z = 0.03f * (i + 1 - (circle_segments >> 1)),
      .w = 1.0f,

      .r = 0.5f,
      .g = 0.0f,
      .b = 0.1f * (i + 1),
      .a = 1.0f
    });
  }
//  renderer.add_mesh(circle_data);

  const float cube_length = 0.75f;
  std::vector<volcano::graphics::vertex> cube_data;
  // Back face
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = -1.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = -1.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = -1.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = -1.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = -1.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = -1.0f
  });
  // Front face
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = 1.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = 1.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = 1.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = 1.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = 1.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 0.0f, .n_z = 1.0f
  });
  // Left face
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = -1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = -1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = -1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = -1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = -1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = -1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  // Right face
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = 1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = 1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = 1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = 1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = 1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 1.0f, .b = 0.5f, .a = 1.0f,
    .n_x = 1.0f, .n_y = 0.0f, .n_z = 0.0f
  });
  // Top face
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 0.5f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 0.5f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 0.5f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 0.5f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 0.5f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 1.0f, .g = 0.5f, .b = 0.5f, .a = 1.0f,
    .n_x = 0.0f, .n_y = 1.0f, .n_z = 0.0f
  });
  // Bottom face
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = -1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = -1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = -1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = -1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = -1.0f, .n_z = 0.0f
  });
  cube_data.push_back({
    .x = -cube_length / 2, .y = -cube_length / 2, .z = -cube_length / 2, .w = 1.0f,
    .r = 0.5f, .g = 0.5f, .b = 1.0f, .a = 1.0f,
    .n_x = 0.0f, .n_y = -1.0f, .n_z = 0.0f
  });
  renderer.add_mesh(cube_data);
}
  
RETRO_API bool retro_load_game(const struct retro_game_info* game) {
  // Initialize vulkan-context
  retro_hw_context_reset_t context_destroy = nullptr;
  static struct retro_hw_render_callback hw_render = {
    .context_type = RETRO_HW_CONTEXT_VULKAN,
    .context_reset = &retro_context_reset,
    .version_major = VK_MAKE_VERSION(1, 0, 18),
    .version_minor = 0,
    .cache_context = true,
    .context_destroy = context_destroy,
  };
  if (!env_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
    return false;
  
  static const struct retro_hw_render_context_negotiation_interface_vulkan iface = {
    RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN,
    RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN_VERSION,
    
    //get_application_info,
    NULL,
    NULL,
  };
  
  env_cb(RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE, (void*)&iface);

  return true;
}

RETRO_API bool retro_load_game_special(
  unsigned game_type,
  const struct retro_game_info *info, size_t num_info
) {
  return false;
}

RETRO_API void retro_get_system_av_info(retro_system_av_info *info) {
  info->geometry = retro_game_geometry {
    .base_width  = WIDTH,
    .base_height = HEIGHT,
    .max_width   = 1920,
    .max_height  = 1080,
  };
  info->timing = retro_system_timing {
    .fps           = 60.0d,
    .sample_rate   = 48000.0d,
  };
}

RETRO_API unsigned retro_get_region(void) {
  return RETRO_REGION_PAL;
}

RETRO_API void retro_run(void) {
  {
    LOG_SCOPE_F(INFO, "Poll Input");
    input_poll_cb();
    
    short input_mask = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);

    {
      LOG_SCOPE_F(INFO, "Input mask: %d", input_mask);
      if(input_mask & (1 << RETRO_DEVICE_ID_JOYPAD_B)) {
        LOG_F(INFO, "RETRO_DEVICE_ID_JOYPAD_B");
      }
      if(input_mask & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) {
        LOG_F(INFO, "RETRO_DEVICE_ID_JOYPAD_UP");
	renderer.move_forward();
      }
      if(input_mask & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) {
        LOG_F(INFO, "RETRO_DEVICE_ID_JOYPAD_DOWN");
	renderer.move_backward();
      }
      if(input_mask & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) {
        LOG_F(INFO, "RETRO_DEVICE_ID_JOYPAD_LEFT");
	renderer.move_left();
      }
      if(input_mask & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
        LOG_F(INFO, "RETRO_DEVICE_ID_JOYPAD_RIGHT");
	renderer.move_right();
      }
      if(input_mask & (1 << RETRO_DEVICE_ID_JOYPAD_A)) {
        LOG_F(INFO, "RETRO_DEVICE_ID_JOYPAD_A");
      }
    }
    // dispatcher.do_stuff
  }

  renderer.render();

  video_cb(RETRO_HW_FRAME_BUFFER_VALID, WIDTH, HEIGHT, 0);
}

RETRO_API void retro_reset(void) {

}

RETRO_API void retro_unload_game(void) {

}

// Memory extraction
RETRO_API size_t retro_get_memory_size(unsigned id) {
  return 0;
}

RETRO_API void* retro_get_memory_data(unsigned id) {
  return nullptr;
}

// Serialization
RETRO_API size_t retro_serialize_size(void) {
  return 0;
}

RETRO_API bool retro_serialize(void *data, size_t size) {
  return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size) {
  return false;
}

// Cheat related
RETRO_API void retro_cheat_reset(void) {
	
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code) {

}

