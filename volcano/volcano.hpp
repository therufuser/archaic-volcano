#ifndef VOLCANO_HPP
  #define VOLCANO_HPP

#include <libretro_vulkan.h>

#define MAX_SYNC 4
#define WIDTH 1280
#define HEIGHT 720

namespace volcano {
  struct buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
  };

  extern retro_hw_render_interface_vulkan* vulkan_if;
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

  void init(retro_hw_render_interface_vulkan*);
  void render();
}

#endif
