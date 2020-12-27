#include "volcano.hpp"

#include <vulkan/vulkan_symbol_wrapper.h>

#include <cstdio>

#define MAX_SYNC 3

struct buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

struct vulkan_data {
  unsigned index;
  unsigned num_swapchain_images;
  uint32_t swapchain_mask;
  struct buffer vbo;
  struct buffer ubo[MAX_SYNC];

  VkPhysicalDeviceMemoryProperties memory_properties;
  VkPhysicalDeviceProperties gpu_properties;

  VkDescriptorSetLayout set_layout;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_set[MAX_SYNC];

  VkPipelineCache pipeline_cache;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  VkPipeline pipeline;

  struct retro_vulkan_image images[MAX_SYNC];
  VkDeviceMemory image_memory[MAX_SYNC];
  VkFramebuffer framebuffers[MAX_SYNC];
  VkCommandPool cmd_pool[MAX_SYNC];
  VkCommandBuffer cmd[MAX_SYNC];
};
static struct vulkan_data vk;

void init_uniform_buffer() {
 
}

void init_vertex_buffer() {

}

void init_command() {

}

void init_descriptor() {

}

void init_render_pass(VkFormat format) {

}

void init_pipeline() {

}

void init_swapchain() {

}

void volcano_init(retro_hw_render_interface_vulkan* vulkan) {
  fprintf(stderr, "volcano_init(): Initialization begun!\n");

  vulkan_symbol_wrapper_init(vulkan->get_instance_proc_addr);
  vulkan_symbol_wrapper_load_core_instance_symbols(vulkan->instance);
  vulkan_symbol_wrapper_load_core_device_symbols(vulkan->device);

  vkGetPhysicalDeviceProperties(vulkan->gpu, &vk.gpu_properties);
  vkGetPhysicalDeviceMemoryProperties(vulkan->gpu, &vk.memory_properties);

  unsigned num_images = 0;
  uint32_t mask = vulkan->get_sync_index_mask(vulkan->handle);
  for (unsigned i = 0; i < 32; i++)
    if (mask & (1u << i))
      num_images = i + 1;
  vk.num_swapchain_images = num_images;
  vk.swapchain_mask = mask;

  init_uniform_buffer();
  init_vertex_buffer();
  init_command();
  init_descriptor();

  VkPipelineCacheCreateInfo pipeline_cache_info = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
  vkCreatePipelineCache(vulkan->device, &pipeline_cache_info, NULL, &vk.pipeline_cache);

  init_render_pass(VK_FORMAT_R8G8B8A8_UNORM);
  init_pipeline();
  init_swapchain();
}
