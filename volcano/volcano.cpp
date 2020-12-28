#include "volcano.hpp"

#include <vulkan/vulkan_symbol_wrapper.h>

#include <cstdio>
#include <cstring>

#define MAX_SYNC 3

struct buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

retro_hw_render_interface_vulkan* vulkan_if;
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

static uint32_t find_memory_type_from_requirements(uint32_t device_requirements, uint32_t host_requirements)
{
  const VkPhysicalDeviceMemoryProperties *props = &vk.memory_properties;
  for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
    if (device_requirements & (1u << i)) {
      if ((props->memoryTypes[i].propertyFlags & host_requirements) == host_requirements) {
        return i;
      }
    }
  }

  return 0;
}

static struct buffer create_buffer(const void *initial, size_t size, VkBufferUsageFlags usage) {
  struct buffer buffer;
  VkDevice device = vulkan_if->device;

  VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  info.usage = usage;
  info.size = size;

  vkCreateBuffer(device, &info, NULL, &buffer.buffer);

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(device, buffer.buffer, &mem_reqs);

  VkMemoryAllocateInfo alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  alloc.allocationSize = mem_reqs.size;

  alloc.memoryTypeIndex = find_memory_type_from_requirements(mem_reqs.memoryTypeBits,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
  );

  vkAllocateMemory(device, &alloc, NULL, &buffer.memory);
  vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0);

  if (initial) {
    void *ptr;
    vkMapMemory(device, buffer.memory, 0, size, 0, &ptr);
    memcpy(ptr, initial, size);
    vkUnmapMemory(device, buffer.memory);
  }

  return buffer;
}

void init_uniform_buffer() {
  for(unsigned i = 0; i < vk.num_swapchain_images; i++) {
    vk.ubo[i] = create_buffer(nullptr, 16 * sizeof(float),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
  }
}

void init_vertex_buffer() {
  // Create a simple colored triangle
  static const float data[] = {
    -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // vec4 position, vec4 color
    -0.5f, +0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    +0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  };

  vk.vbo = create_buffer(data, sizeof(data), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void init_command() {
  VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };

  pool_info.queueFamilyIndex = vulkan_if->queue_index;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  for (unsigned i = 0; i < vk.num_swapchain_images; i++) {
    vkCreateCommandPool(vulkan_if->device, &pool_info, NULL, &vk.cmd_pool[i]);
    info.commandPool = vk.cmd_pool[i];
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;
    vkAllocateCommandBuffers(vulkan_if->device, &info, &vk.cmd[i]);
  }
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
  vulkan_if = vulkan;
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
