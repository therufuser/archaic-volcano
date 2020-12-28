#include "volcano.hpp"

#include <vulkan/vulkan_symbol_wrapper.h>

#include <cstdio>
#include <cstring>
#include <cmath>

#define MAX_SYNC 4

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
  VkDevice device = vulkan_if->device;

  VkDescriptorSetLayoutBinding binding = {0};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  binding.pImmutableSamplers = NULL;

  const VkDescriptorPoolSize pool_sizes[1] = {
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vk.num_swapchain_images },
  };

  VkDescriptorSetLayoutCreateInfo set_layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  set_layout_info.bindingCount = 1;
  set_layout_info.pBindings = &binding;
  vkCreateDescriptorSetLayout(device, &set_layout_info, NULL, &vk.set_layout);

  VkPipelineLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  layout_info.setLayoutCount = 1;
  layout_info.pSetLayouts = &vk.set_layout;
  vkCreatePipelineLayout(device, &layout_info, NULL, &vk.pipeline_layout);

  VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
  pool_info.maxSets = vk.num_swapchain_images;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = pool_sizes;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  vkCreateDescriptorPool(device, &pool_info, NULL, &vk.desc_pool);

  VkDescriptorSetAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
  alloc_info.descriptorPool = vk.desc_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &vk.set_layout;
  for (unsigned i = 0; i < vk.num_swapchain_images; i++) {
    vkAllocateDescriptorSets(device, &alloc_info, &vk.desc_set[i]);

    VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    VkDescriptorBufferInfo buffer_info;

    write.dstSet = vk.desc_set[i];
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer_info;

    buffer_info.buffer = vk.ubo[i].buffer;
    buffer_info.offset = 0;
    buffer_info.range = 16 * sizeof(float);

    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
  }
}

void init_render_pass(VkFormat format) {
  VkAttachmentDescription attachment = { 0 };
  attachment.format = format;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
  VkSubpassDescription subpass = { 0 };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_ref;

  VkRenderPassCreateInfo rp_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
  rp_info.attachmentCount = 1;
  rp_info.pAttachments = &attachment;
  rp_info.subpassCount = 1;
  rp_info.pSubpasses = &subpass;
  vkCreateRenderPass(vulkan_if->device, &rp_info, NULL, &vk.render_pass);
}

static VkShaderModule create_shader_module(const uint32_t *data, size_t size)
{
  VkShaderModuleCreateInfo module_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  VkShaderModule module;
  module_info.codeSize = size;
  module_info.pCode = data;
  vkCreateShaderModule(vulkan_if->device, &module_info, NULL, &module);
  return module;
}

void init_pipeline() {
  VkDevice device = vulkan_if->device;

  VkPipelineInputAssemblyStateCreateInfo input_assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkVertexInputAttributeDescription attributes[2] = {{ 0 }};
  attributes[0].location = 0;
  attributes[0].binding = 0;
  attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[0].offset = 0;
  attributes[1].location = 1;
  attributes[1].binding = 0;
  attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[1].offset = 4 * sizeof(float);

  VkVertexInputBindingDescription binding = { 0 };
  binding.binding = 0;
  binding.stride = sizeof(float) * 8;
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkPipelineVertexInputStateCreateInfo vertex_input = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  vertex_input.vertexBindingDescriptionCount = 1;
  vertex_input.pVertexBindingDescriptions = &binding;
  vertex_input.vertexAttributeDescriptionCount = 2;
  vertex_input.pVertexAttributeDescriptions = attributes;

  VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  raster.polygonMode = VK_POLYGON_MODE_FILL;
  raster.cullMode = VK_CULL_MODE_BACK_BIT;
  raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  raster.depthClampEnable = false;
  raster.rasterizerDiscardEnable = false;
  raster.depthBiasEnable = false;
  raster.lineWidth = 1.0f;

  VkPipelineColorBlendAttachmentState blend_attachment = { 0 };
  blend_attachment.blendEnable = false;
  blend_attachment.colorWriteMask = 0xf;

  VkPipelineColorBlendStateCreateInfo blend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
  blend.attachmentCount = 1;
  blend.pAttachments = &blend_attachment;

  VkPipelineViewportStateCreateInfo viewport = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  VkPipelineDepthStencilStateCreateInfo depth_stencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  depth_stencil.depthTestEnable = false;
  depth_stencil.depthWriteEnable = false;
  depth_stencil.depthBoundsTestEnable = false;
  depth_stencil.stencilTestEnable = false;

  VkPipelineMultisampleStateCreateInfo multisample = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  static const VkDynamicState dynamics[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamic = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
  dynamic.pDynamicStates = dynamics;
  dynamic.dynamicStateCount = sizeof(dynamics) / sizeof(dynamics[0]);

  VkPipelineShaderStageCreateInfo shader_stages[2] = {
    { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
    { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
  };

  static const uint32_t triangle_vert[] =
    #include "shaders/triangle.vert.inc"
  ;

  static const uint32_t triangle_frag[] =
    #include "shaders/triangle.frag.inc"
  ;

  shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shader_stages[0].module = create_shader_module(triangle_vert, sizeof(triangle_vert));
  shader_stages[0].pName = "main";
  shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shader_stages[1].module = create_shader_module(triangle_frag, sizeof(triangle_frag));
  shader_stages[1].pName = "main";

  VkGraphicsPipelineCreateInfo pipe = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
  pipe.stageCount = 2;
  pipe.pStages = shader_stages;
  pipe.pVertexInputState = &vertex_input;
  pipe.pInputAssemblyState = &input_assembly;
  pipe.pRasterizationState = &raster;
  pipe.pColorBlendState = &blend;
  pipe.pMultisampleState = &multisample;
  pipe.pViewportState = &viewport;
  pipe.pDepthStencilState = &depth_stencil;
  pipe.pDynamicState = &dynamic;
  pipe.renderPass = vk.render_pass;
  pipe.layout = vk.pipeline_layout;

  vkCreateGraphicsPipelines(vulkan_if->device, vk.pipeline_cache, 1, &pipe, NULL, &vk.pipeline);

  vkDestroyShaderModule(device, shader_stages[0].module, NULL);
  vkDestroyShaderModule(device, shader_stages[1].module, NULL);
}

void init_swapchain() {
  VkDevice device = vulkan_if->device;

  for (unsigned i = 0; i < vk.num_swapchain_images; i++) {
    VkImageCreateInfo image = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    image.imageType = VK_IMAGE_TYPE_2D;
    image.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    image.format = VK_FORMAT_R8G8B8A8_UNORM;
    image.extent.width = 1280;
    image.extent.height = 720;
    image.extent.depth = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image.mipLevels = 1;
    image.arrayLayers = 1;

    vkCreateImage(device, &image, NULL, &vk.images[i].create_info.image);

    VkMemoryAllocateInfo alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryRequirements mem_reqs;

    vkGetImageMemoryRequirements(device, vk.images[i].create_info.image, &mem_reqs);
    alloc.allocationSize = mem_reqs.size;
    alloc.memoryTypeIndex = find_memory_type_from_requirements(
      mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vkAllocateMemory(device, &alloc, NULL, &vk.image_memory[i]);
    vkBindImageMemory(device, vk.images[i].create_info.image, vk.image_memory[i], 0);

    vk.images[i].create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vk.images[i].create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vk.images[i].create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    vk.images[i].create_info.subresourceRange.baseMipLevel = 0;
    vk.images[i].create_info.subresourceRange.baseArrayLayer = 0;
    vk.images[i].create_info.subresourceRange.levelCount = 1;
    vk.images[i].create_info.subresourceRange.layerCount = 1;
    vk.images[i].create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vk.images[i].create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    vk.images[i].create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    vk.images[i].create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    vk.images[i].create_info.components.a = VK_COMPONENT_SWIZZLE_A;

    vkCreateImageView(device, &vk.images[i].create_info,
      NULL, &vk.images[i].image_view);
    vk.images[i].image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkFramebufferCreateInfo fb_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    fb_info.renderPass = vk.render_pass;
    fb_info.attachmentCount = 1;
    fb_info.pAttachments = &vk.images[i].image_view;
    fb_info.width = 1280;
    fb_info.height = 720;
    fb_info.layers = 1;

    vkCreateFramebuffer(device, &fb_info, NULL, &vk.framebuffers[i]);
  }
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

void update_ubo(void)
{
  static unsigned frame;
  float c = cosf(frame * 0.01f);
  float s = sinf(frame * 0.01f);
  frame++;

  float tmp[16] = {0.0f};
  tmp[ 0] = c;
  tmp[ 1] = s;
  tmp[ 4] = -s;
  tmp[ 5] = c;
  tmp[10] = 1.0f;
  tmp[15] = 1.0f;

  float *mvp = NULL;
  vkMapMemory(vulkan_if->device, vk.ubo[vk.index].memory,
    0, 16 * sizeof(float), 0, (void**)&mvp);
  memcpy(mvp, tmp, sizeof(tmp));
  vkUnmapMemory(vulkan_if->device, vk.ubo[vk.index].memory);
}

void volcano_render() {
  vulkan_if->wait_sync_index(vulkan_if->handle);
  vk.index = vulkan_if->get_sync_index(vulkan_if->handle);

  update_ubo();

  VkCommandBuffer cmd = vk.cmd[vk.index];

  VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkResetCommandBuffer(cmd, 0);
  vkBeginCommandBuffer(cmd, &begin_info);

  VkImageMemoryBarrier prepare_rendering = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
  prepare_rendering.srcAccessMask = 0;
  prepare_rendering.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  prepare_rendering.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  prepare_rendering.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  prepare_rendering.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  prepare_rendering.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  prepare_rendering.image = vk.images[vk.index].create_info.image;
  prepare_rendering.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  prepare_rendering.subresourceRange.levelCount = 1;
  prepare_rendering.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(cmd,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    false, 
    0, NULL,
    0, NULL,
    1, &prepare_rendering
  );

  VkClearValue clear_value;
  clear_value.color.float32[0] = 0.8f;
  clear_value.color.float32[1] = 0.6f;
  clear_value.color.float32[2] = 0.2f;
  clear_value.color.float32[3] = 1.0f;

  VkRenderPassBeginInfo rp_begin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
  rp_begin.renderPass = vk.render_pass;
  rp_begin.framebuffer = vk.framebuffers[vk.index];
  rp_begin.renderArea.extent.width = 1280;
  rp_begin.renderArea.extent.height = 720;
  rp_begin.clearValueCount = 1;
  rp_begin.pClearValues = &clear_value;
  vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    vk.pipeline_layout, 0,
    1, &vk.desc_set[vk.index], 0, NULL);

  VkViewport vp = { 0 };
  vp.x = 0.0f;
  vp.y = 0.0f;
  vp.width = 1280;
  vp.height = 720;
  vp.minDepth = 0.0f;
  vp.maxDepth = 1.0f;
  vkCmdSetViewport(cmd, 0, 1, &vp);

  VkRect2D scissor;
  memset(&scissor, 0, sizeof(scissor));
  scissor.extent.width = 1280;
  scissor.extent.height = 720;
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &vk.vbo.buffer, &offset);

  vkCmdDraw(cmd, 3, 1, 0, 0);

  vkCmdEndRenderPass(cmd);

  VkImageMemoryBarrier prepare_presentation = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
  prepare_presentation.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  prepare_presentation.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  prepare_presentation.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  prepare_presentation.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  prepare_presentation.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  prepare_presentation.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  prepare_presentation.image = vk.images[vk.index].create_info.image;
  prepare_presentation.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  prepare_presentation.subresourceRange.levelCount = 1;
  prepare_presentation.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    false,
    0, NULL,
    0, NULL,
    1, &prepare_presentation
  );

  vkEndCommandBuffer(cmd);

  vulkan_if->set_image(vulkan_if->handle, &vk.images[vk.index], 0, NULL, VK_QUEUE_FAMILY_IGNORED);
  vulkan_if->set_command_buffers(vulkan_if->handle, 1, &vk.cmd[vk.index]);
}
