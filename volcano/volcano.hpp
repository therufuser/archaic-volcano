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

  class renderer {
    private:
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

      uint32_t find_memory_type_from_requirements(uint32_t device_requirements, uint32_t host_requirements);
      struct buffer create_buffer(const void *initial, size_t size, VkBufferUsageFlags usage);
      void init_uniform_buffer();
      void init_vertex_buffer();
      void init_command();
      void init_descriptor();
      void init_render_pass(VkFormat format);
      VkShaderModule create_shader_module(const uint32_t *data, size_t size);
      void init_pipeline();
      void init_swapchain();
      void update_ubo();

   public:
      void init(retro_hw_render_interface_vulkan*);
      void render();
  };
}

#endif
