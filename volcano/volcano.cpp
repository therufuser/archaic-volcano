#include "volcano.hpp"
#include "renderer/mesh.hpp"

#include <vulkan/vulkan_symbol_wrapper.h>

#include <loguru.hpp>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <cstdio>
#include <cstring>
#include <cmath>

namespace volcano {
  static retro_hw_render_interface_vulkan* vulkan_if;

  uint32_t renderer::find_memory_type_from_requirements(uint32_t device_requirements, uint32_t host_requirements) {
    const VkPhysicalDeviceMemoryProperties *props = &this->memory_properties;
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
      if (device_requirements & (1u << i)) {
	if ((props->memoryTypes[i].propertyFlags & host_requirements) == host_requirements) {
	  return i;
	}
      }
    }

    return 0;
  }

  struct buffer renderer::create_buffer(const void *initial, size_t size, VkBufferUsageFlags usage) {
    struct buffer buffer;
    VkDevice device = vulkan_if->device;

    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
    };
    vkCreateBuffer(device, &info, nullptr, &buffer.buffer);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_reqs.size,
      .memoryTypeIndex = this->find_memory_type_from_requirements(mem_reqs.memoryTypeBits,
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      ),
    };

    vkAllocateMemory(device, &alloc, nullptr, &buffer.memory);
    vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0);

    if (initial) {
      void *ptr;
      vkMapMemory(device, buffer.memory, 0, size, 0, &ptr);
      memcpy(ptr, initial, size);
      vkUnmapMemory(device, buffer.memory);
    }

    return buffer;
  }

  void renderer::init_uniform_buffer() {
    for(unsigned i = 0; i < this->num_swapchain_images; i++) {
      this->ubo[i] = create_buffer(
	nullptr, 16 * sizeof(float),
	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
      );
    }
  }

  void renderer::init_command() {
    VkCommandPoolCreateInfo pool_info = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = vulkan_if->queue_index,
    };

    for (unsigned i = 0; i < this->num_swapchain_images; i++) {
      vkCreateCommandPool(vulkan_if->device, &pool_info, nullptr, &this->cmd_pool[i]);

      VkCommandBufferAllocateInfo info = {
	.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = this->cmd_pool[i],
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
      };
      vkAllocateCommandBuffers(vulkan_if->device, &info, &this->cmd[i]);
    }
  }

  void renderer::init_descriptor() {
    VkDevice device = vulkan_if->device;

    // Initialize descriptor pool
    const VkDescriptorPoolSize pool_sizes[1] = {
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->num_swapchain_images },
    };

    VkDescriptorPoolCreateInfo pool_info = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets       = this->num_swapchain_images,
      .poolSizeCount = 1,
      .pPoolSizes    = pool_sizes,
    };
    vkCreateDescriptorPool(device, &pool_info, nullptr, &this->desc_pool);

    // Define descriptor set
    VkDescriptorSetLayoutBinding binding = {
      .binding            = 0,
      .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount    = 1,
      .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo set_layout_info = {
      .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings    = &binding,
    };
    vkCreateDescriptorSetLayout(device, &set_layout_info, nullptr, &this->set_layout);

    VkDescriptorSetAllocateInfo alloc_info = {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool     = this->desc_pool,
      .descriptorSetCount = 1,
      .pSetLayouts        = &this->set_layout,
    };
    for (unsigned i = 0; i < this->num_swapchain_images; i++) {
      vkAllocateDescriptorSets(device, &alloc_info, &this->desc_set[i]);

      VkDescriptorBufferInfo buffer_info = {
        .buffer = this->ubo[i].buffer,
        .offset = 0,
        .range  = 16 * sizeof(float),
      };

      VkWriteDescriptorSet write = {
	.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = this->desc_set[i],
        .dstBinding      = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = &buffer_info,
      };

      vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    // Create pipeline-layout for descriptor set
    VkPipelineLayoutCreateInfo layout_info = {
      .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts    = &this->set_layout,
    };
    vkCreatePipelineLayout(device, &layout_info, nullptr, &this->pipeline_layout);
  }

  void renderer::init_render_pass(VkFormat format) {
    VkAttachmentDescription attachment = {
      .format         = format,
      .samples        = VK_SAMPLE_COUNT_1_BIT,
      .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference color_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpass = {
      .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &color_ref,
    };

    VkRenderPassCreateInfo rp_info = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments    = &attachment,
      .subpassCount    = 1,
      .pSubpasses      = &subpass,
    };
    vkCreateRenderPass(vulkan_if->device, &rp_info, nullptr, &this->render_pass);
  }

  VkShaderModule renderer::create_shader_module(const uint32_t *data, size_t size) {
    VkShaderModule module;

    VkShaderModuleCreateInfo module_info = {
      .sType     = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize  = size,
      .pCode     = data,
    };
    vkCreateShaderModule(vulkan_if->device, &module_info, nullptr, &module);

    return module;
  }

  void renderer::init_pipeline() {
    VkDevice device = vulkan_if->device;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
      .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkVertexInputAttributeDescription attributes[3] = {
      {
        .location = 0,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset   = offsetof(graphics::vertex, x),
      }, {
        .location = 1,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset   = offsetof(graphics::vertex, r),
      }, {
        .location = 2,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = offsetof(graphics::vertex, n_x),
      },
    };

    VkVertexInputBindingDescription binding = {
      .binding   = 0,
      .stride    = sizeof(volcano::graphics::vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
      .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount   = 1,
      .pVertexBindingDescriptions      = &binding,
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions    = attributes,
    };

    VkPipelineRasterizationStateCreateInfo raster = {
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable        = false,
      .rasterizerDiscardEnable = false,
      .polygonMode             = VK_POLYGON_MODE_FILL,
      .cullMode                = VK_CULL_MODE_BACK_BIT,
      .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable         = false,
      .lineWidth               = 1.0f,
    };

    VkPipelineColorBlendAttachmentState blend_attachment = {
      .blendEnable = false,
      .colorWriteMask = 0xf,
    };

    VkPipelineColorBlendStateCreateInfo blend = {
      .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments    = &blend_attachment,
    };

    VkPipelineViewportStateCreateInfo viewport = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount  = 1,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
      .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable       = false,
      .depthWriteEnable      = false,
      .depthBoundsTestEnable = false,
      .stencilTestEnable     = false,
    };

    VkPipelineMultisampleStateCreateInfo multisample = {
      .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    static const VkDynamicState dynamics[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic = {
      .sType      = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = sizeof(dynamics) / sizeof(dynamics[0]),
      .pDynamicStates = dynamics,
    };

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

    shader_stages[0] = {
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = create_shader_module(triangle_vert, sizeof(triangle_vert)),
      .pName = "main",
    };
    shader_stages[1] = {
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = create_shader_module(triangle_frag, sizeof(triangle_frag)),
      .pName = "main",
    };

    VkGraphicsPipelineCreateInfo pipe = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport,
      .pRasterizationState = &raster,
      .pMultisampleState = &multisample,
      .pDepthStencilState = &depth_stencil,
      .pColorBlendState = &blend,
      .pDynamicState = &dynamic,
      .layout = this->pipeline_layout,
      .renderPass = this->render_pass,
    };
    vkCreateGraphicsPipelines(vulkan_if->device, this->pipeline_cache, 1, &pipe, nullptr, &this->pipeline);

    vkDestroyShaderModule(device, shader_stages[0].module, nullptr);
    vkDestroyShaderModule(device, shader_stages[1].module, nullptr);
  }

  void renderer::init_swapchain() {
    VkDevice device = vulkan_if->device;

    for (unsigned i = 0; i < this->num_swapchain_images; i++) {
      VkImageCreateInfo image = {
	.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R8G8B8A8_UNORM,
        .extent        = {
	  .width  = WIDTH,
          .height = HEIGHT,
          .depth  = 1,
        },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         =
	  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
	  VK_IMAGE_USAGE_SAMPLED_BIT |
	  VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      };
      VkImage newImage;
      vkCreateImage(device, &image, nullptr, &newImage);

      VkMemoryRequirements mem_reqs;
      vkGetImageMemoryRequirements(device, newImage, &mem_reqs);
 
      VkMemoryAllocateInfo alloc = {
	.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	.allocationSize  = mem_reqs.size,
	.memoryTypeIndex = find_memory_type_from_requirements(
	  mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	),
      };
      vkAllocateMemory(device, &alloc, nullptr, &this->image_memory[i]);
      vkBindImageMemory(device, newImage, this->image_memory[i], 0);

      this->images[i].create_info = {
	.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	.image            = newImage,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = VK_FORMAT_R8G8B8A8_UNORM,
        .components       = {
	  .r = VK_COMPONENT_SWIZZLE_R,
	  .g = VK_COMPONENT_SWIZZLE_G,
	  .b = VK_COMPONENT_SWIZZLE_B,
	  .a = VK_COMPONENT_SWIZZLE_A,
	},
        .subresourceRange = {
	  .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
	  .baseMipLevel   = 0,
	  .levelCount     = 1,
	  .baseArrayLayer = 0,
	  .layerCount     = 1,
	},
      };

      vkCreateImageView(
	device, &this->images[i].create_info,
	nullptr, &this->images[i].image_view
      );
      this->images[i].image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      VkFramebufferCreateInfo fb_info = {
	.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = this->render_pass,
        .attachmentCount = 1,
        .pAttachments    = &this->images[i].image_view,
        .width           = WIDTH,
        .height          = HEIGHT,
        .layers          = 1,
      };
      vkCreateFramebuffer(device, &fb_info, nullptr, &this->framebuffers[i]);
    }
  }

  void renderer::init(retro_hw_render_interface_vulkan* vulkan) {
    vulkan_if = vulkan;
    fprintf(stderr, "volcano_init(): Initialization begun!\n");

    vulkan_symbol_wrapper_init(vulkan->get_instance_proc_addr);
    vulkan_symbol_wrapper_load_core_instance_symbols(vulkan->instance);
    vulkan_symbol_wrapper_load_core_device_symbols(vulkan->device);

    vkGetPhysicalDeviceProperties(vulkan->gpu, &this->gpu_properties);
    vkGetPhysicalDeviceMemoryProperties(vulkan->gpu, &this->memory_properties);

    unsigned num_images = 0;
    uint32_t mask = vulkan->get_sync_index_mask(vulkan->handle);
    for (unsigned i = 0; i < 32; i++)
      if (mask & (1u << i))
	num_images = i + 1;
    this->num_swapchain_images = num_images;
    this->swapchain_mask = mask;

    init_uniform_buffer();
    init_command();
    init_descriptor();

    VkPipelineCacheCreateInfo pipeline_cache_info = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
    vkCreatePipelineCache(vulkan->device, &pipeline_cache_info, nullptr, &this->pipeline_cache);

    init_render_pass(VK_FORMAT_R8G8B8A8_UNORM);
    init_pipeline();
    init_swapchain();
  }

  void renderer::update_ubo(void) {
    static unsigned frame;

    glm::mat4 view = glm::rotate(glm::mat4(1.0f), frame * 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::rotate(view, glm::pi<float>() / 5, glm::vec3(1.0f, 0.0f, 0.0f));

    float *mvp = nullptr;
    vkMapMemory(
      vulkan_if->device, this->ubo[this->index].memory,
      0, 16 * sizeof(float), 0, (void**)&mvp
    );
    memcpy(mvp, &view, sizeof(view));
    vkUnmapMemory(vulkan_if->device, this->ubo[this->index].memory);

    frame++;
  }

  void renderer::render() {
    vulkan_if->wait_sync_index(vulkan_if->handle);
    this->index = vulkan_if->get_sync_index(vulkan_if->handle);

    update_ubo();

    VkCommandBuffer cmd = this->cmd[this->index];

    VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier prepare_rendering = {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext               = nullptr,
      .srcAccessMask       = 0,
      .dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
      .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image               = this->images[this->index].create_info.image,
      .subresourceRange    = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      }
    };
    vkCmdPipelineBarrier(cmd,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      false, 
      0, nullptr,
      0, nullptr,
      1, &prepare_rendering
    );

    VkClearValue clear_value = {
      .color = {
	.float32 = {
	  0.8f,
	  0.6f,
	  0.2f,
	  1.0f,
	},
      },
    };

    VkRenderPassBeginInfo rp_begin = {
      .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext             = nullptr,
      .renderPass        = this->render_pass,
      .framebuffer       = this->framebuffers[this->index],
      .renderArea = {
	{     0,      0 },
	{ WIDTH, HEIGHT },
      },
      .clearValueCount   = 1,
      .pClearValues      = &clear_value,
    };
    vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->pipeline_layout, 0,
      1, &this->desc_set[this->index], 0, nullptr
    );

    VkViewport vp = {
      .x        = 0.0f,
      .y        = 0.0f,
      .width    = WIDTH,
      .height   = HEIGHT,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D scissor = {
      {     0,      0 },
      { WIDTH, HEIGHT },
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkDeviceSize offset = 0;

    for(mesh& cur_mesh: meshes) {
      vkCmdBindVertexBuffers(cmd, 0, 1, cur_mesh.get_buffer(), &offset);
      vkCmdDraw(cmd, cur_mesh.get_size(), 1, 0, 0);
      LOG_F(MAX, "Added draw-call for buffer %d with %d vertices", cur_mesh.get_buffer(), cur_mesh.get_size());
    }

    vkCmdEndRenderPass(cmd);

    VkImageMemoryBarrier prepare_presentation = {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext               = nullptr,
      .srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
      .oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image               = this->images[this->index].create_info.image,
      .subresourceRange    = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      }
    };
    vkCmdPipelineBarrier(cmd,
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      false,
      0, nullptr,
      0, nullptr,
      1, &prepare_presentation
    );

    vkEndCommandBuffer(cmd);

    vulkan_if->set_image(vulkan_if->handle, &this->images[this->index], 0, nullptr, VK_QUEUE_FAMILY_IGNORED);
    vulkan_if->set_command_buffers(vulkan_if->handle, 1, &this->cmd[this->index]);
  }

  void renderer::add_mesh(std::vector<graphics::vertex>& vertices) {
    LOG_F(MAX, "Add new mesh (size=%d)", vertices.size());
    meshes.push_back(mesh(this, vertices.data(), vertices.size()));
  }
}
