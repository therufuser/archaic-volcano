#include <libretro.h>

RETRO_API unsigned int retro_api_version () {
  return RETRO_API_VERSION;
}

RETRO_API void retro_set_environment(retro_environment_t cb) {
  bool no_rom = true;
  cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

const int w = 1280;
const int h = 720;
static unsigned short buffer[w * h];

RETRO_API void retro_init() {

}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

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

static const char* library_name = "Archaic Volcano";
static const char* library_version = "v0.0.1";
static const char* valid_extensions = "";

RETRO_API void retro_get_system_info(retro_system_info* info) {
  info->library_name     = library_name;
  info->library_version  = library_version;
  info->valid_extensions = valid_extensions;
  info->need_fullpath    = false;
  info->block_extract    = false;
}

RETRO_API bool retro_load_game(const struct retro_game_info* game) {
  return true;
}

RETRO_API bool retro_load_game_special(
  unsigned game_type,
  const struct retro_game_info *info, size_t num_info
) {
  return false;
}

RETRO_API void retro_get_system_av_info(retro_system_av_info *info) {
  info->geometry.base_width  = 1280;
  info->geometry.base_height = 720;
  info->geometry.max_width   = 1920;
  info->geometry.max_height  = 1080;
  info->timing.fps           = 60.0d;
  info->timing.sample_rate   = 48000.0d;
}

RETRO_API size_t retro_serialize_size(void) {
  return 0;
}

RETRO_API void retro_reset(void) {

}

RETRO_API void retro_unload_game(void) {

}

RETRO_API unsigned retro_get_region(void) {
  return RETRO_REGION_PAL;
}

RETRO_API void* retro_get_memory_data(unsigned id) {
  return nullptr;
}

RETRO_API size_t retro_get_memory_size(unsigned id) {
  return 0;
}

unsigned int abs(int x) {
  if(x < 0)
    return -x;

  return x;
}

RETRO_API void retro_run(void) {
  for(int y = 0; y < h; y++) {
    for(int x = 0; x < w; x++) {
      buffer[x + w * y] = 0;
      if(abs(x - y) > 10) {
        buffer[x + w * y] = 65535;
      }
    }
  }

  video_cb(buffer, w, h, sizeof(unsigned short) * w);
}

RETRO_API bool retro_serialize(void *data, size_t size) {
  return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size) {
  return false;
}

RETRO_API void retro_cheat_reset(void) {
	
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code) {

}

RETRO_API void retro_deinit() {

}
