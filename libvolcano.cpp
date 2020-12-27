#include <libretro.h>

RETRO_API unsigned int retro_api_version () {
  return 0;
}

RETRO_API void retro_set_environment(retro_environment_t) {

}

RETRO_API void retro_init() {

}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t) {

}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t) {

}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t) {

}

RETRO_API void retro_set_input_poll(retro_input_poll_t) {

}

RETRO_API void retro_set_input_state(retro_input_state_t) {

}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device) {

}

RETRO_API void retro_get_system_info(retro_system_info *info) {

}

RETRO_API void retro_get_system_av_info(retro_system_av_info *info) {

}

RETRO_API size_t retro_serialize_size(void) {
  return 0;
}

RETRO_API void retro_reset(void) {

}

RETRO_API bool retro_load_game(const struct retro_game_info *game) {
  return false;
}

RETRO_API bool retro_load_game_special(
  unsigned game_type,
  const struct retro_game_info *info, size_t num_info
) {
  return false;
}

RETRO_API void retro_unload_game(void) {

}

RETRO_API unsigned retro_get_region(void) {
  return 0;
}

RETRO_API void* retro_get_memory_data(unsigned id) {
  return 0;
}

RETRO_API size_t retro_get_memory_size(unsigned id) {
  return 0;
}

RETRO_API void retro_run(void) {

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
