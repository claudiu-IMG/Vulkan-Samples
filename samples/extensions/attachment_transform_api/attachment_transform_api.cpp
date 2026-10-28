/* Copyright (c) 2026, Claudiu Ciungan
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "attachment_transform_api.h"

#include "common/strings.h"
#define VK_EXT_attachment_transform 1
#define VK_KHR_extended_flags 1


AttachmentTransformApi::AttachmentTransformApi()
{
}

AttachmentTransformApi::~AttachmentTransformApi()
{
	if (has_device())
	{
		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
	}
}

void AttachmentTransformApi::request_device_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const
{
	LOGI("clci request_device_extensions called");
	ApiVulkanSample::request_device_extensions(requested_extensions);
	requested_extensions[VK_EXT_ATTACHMENT_TRANSFORM_EXTENSION_NAME] = vkb::RequestMode::Required;
	//Dependency of VK_EXT_attachment_transform, provides the 64-bit image usage flags
	requested_extensions[VK_KHR_EXTENDED_FLAGS_EXTENSION_NAME] = vkb::RequestMode::Required;
}

void AttachmentTransformApi::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	LOGI("clci request_gpu_features called");

	// clci todo The Vulakn HPP headers are not up to date so it will thrwo compile error
	// REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceAttachmentTransformFeaturesEXT, attachmentTransform);
	// REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDeviceExtendedFlagsFeaturesKHR, extendedFlags);
}

bool AttachmentTransformApi::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	LOGI("clci prepare called");
	create_pipeline();
	build_command_buffers();

	prepared = true;
	return true;
}

void AttachmentTransformApi::setup_render_pass()
{
	VkAttachmentDescription color_attachment{
	    .format         = get_render_context().get_format(),
	    .samples        = VK_SAMPLE_COUNT_1_BIT,
	    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
	    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
	    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
	    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

	VkAttachmentReference color_reference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpass{
	    .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &color_reference};

	// Wait for the presentation engine to release the swapchain image before writing to it
	VkSubpassDependency dependency{
	    .srcSubpass    = VK_SUBPASS_EXTERNAL,
	    .dstSubpass    = 0,
	    .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

	// Allows to begin the render pass with an attachment transform. The transform itself is only given when recording,
	// so the render pass and the pipelines created with it can be used with any transform
	VkRenderPassCreateInfo render_pass_create_info{
	    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    .flags           = VK_RENDER_PASS_CREATE_ATTACHMENT_TRANSFORM_BIT_EXT,		//clci add flag for ATTACHMENT_TRANSFORM_BIT_EXT
	    .attachmentCount = 1,
	    .pAttachments    = &color_attachment,
	    .subpassCount    = 1,
	    .pSubpasses      = &subpass,
	    .dependencyCount = 1,
	    .pDependencies   = &dependency};

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &render_pass));
}

void AttachmentTransformApi::setup_framebuffer()
{
	// Attachments are in natural orientation: the framebuffers have the extent of the swapchain images
	const VkExtent2D &extent = get_render_context().get_swapchain().get_extent();

	for (VkFramebuffer framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(get_device().get_handle(), framebuffer, nullptr);
	}
	framebuffers.resize(swapchain_buffers.size());

	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		VkFramebufferCreateInfo framebuffer_create_info{
		    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		    .renderPass      = render_pass,
		    .attachmentCount = 1,
		    .pAttachments    = &swapchain_buffers[i].view,
		    .width           = extent.width,
		    .height          = extent.height,
		    .layers          = 1};
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &framebuffers[i]));
	}
}

void AttachmentTransformApi::create_pipeline()
{
	VkPipelineLayoutCreateInfo pipeline_layout_create_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

	const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
	    load_shader("attachment_transform_api", "triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("attachment_transform_api", "triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};
		//load_shader("triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
		//load_shader("triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	VkPipelineVertexInputStateCreateInfo   vertex_input_state   = vkb::initializers::pipeline_vertex_input_state_create_info();
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineViewportStateCreateInfo      viewport_state       = vkb::initializers::pipeline_viewport_state_create_info(1, 1);
	VkPipelineRasterizationStateCreateInfo rasterization_state  = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	VkPipelineMultisampleStateCreateInfo   multisample_state    = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);
	VkPipelineColorBlendAttachmentState    blend_attachment     = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo    color_blend_state    = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);
	const std::array<VkDynamicState, 2>    dynamic_states       = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo       dynamic_state        = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_states.data(), vkb::to_u32(dynamic_states.size()));

	// Nothing specific to the attachment transform: with render pass objects, the flag is set on the render pass.
	// With dynamic rendering, the pipeline would be created with VK_PIPELINE_CREATE_2_ATTACHMENT_TRANSFORM_BIT_EXT instead
	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass);
	pipeline_create_info.stageCount                   = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

void AttachmentTransformApi::build_command_buffers()
{
	// The swapchain transform rotates the logical orientation into the natural orientation of the swapchain images.
	// Passing it as attachment transform makes the implementation apply that rotation while rendering.
	const VkSurfaceTransformFlagBitsKHR transform = get_render_context().get_swapchain().get_transform();
	VkAttachmentTransformInfoEXT        transform_info{.sType = VK_STRUCTURE_TYPE_ATTACHMENT_TRANSFORM_INFO_EXT, .transform = transform};

	// Render area, viewport and scissor are in logical orientation:
	// the extent of the swapchain images with width and height swapped for 90 and 270 degree rotations
	VkExtent2D extent = get_render_context().get_swapchain().get_extent();
	if (transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
	{
		std::swap(extent.width, extent.height);
	}
	const VkRect2D   render_area{{0, 0}, extent};
	const VkViewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};

	VkClearValue clear_value{.color = default_clear_color};

	VkRenderPassBeginInfo render_pass_begin_info{
	    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    .pNext           = &transform_info,
	    .renderPass      = render_pass,
	    .renderArea      = render_area,
	    .clearValueCount = 1,
	    .pClearValues    = &clear_value};

	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &render_area);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);
		// The UI is drawn in the same render pass instance, so it's rotated as well
		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void AttachmentTransformApi::handle_rotation()
{
	VkSurfaceCapabilitiesKHR surface_capabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(get_device().get_gpu().get_handle(), get_surface(), &surface_capabilities));

	// Pre-rotation: the swapchain preTransform matches the current transform of the surface, so the presentation engine
	// doesn't rotate the images. Attachment transforms only support rotations, mirror transforms are left to the presentation engine.
	const bool is_rotation = surface_capabilities.currentTransform &
	                         (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR);

	//clci todo Update the condition in order to simulate a screen rotation
	const VkSurfaceTransformFlagBitsKHR pre_transform = pre_rotate && is_rotation ? surface_capabilities.currentTransform : VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	// Rotations by 90 and 270 degrees change the surface extent and are handled first as a resize, which keeps the previous
	// transform. Once the extent is up to date, or for 180 degree rotations that keep the extent, the transform is updated here.
	const VkExtent2D &surface_extent = get_render_context().get_surface_extent();
	if (pre_transform != get_render_context().get_swapchain().get_transform() &&
	    surface_capabilities.currentExtent.width == surface_extent.width && surface_capabilities.currentExtent.height == surface_extent.height)
	{
		get_device().wait_idle();
		// The swapchain images are created in natural orientation, i.e. with width and height swapped for 90 and 270 degree rotations
		get_render_context().update_swapchain(surface_extent, pre_transform);
		create_swapchain_buffers();
		setup_framebuffer();
		rebuild_command_buffers();
	}
}

void AttachmentTransformApi::render(float)
{
	if (!prepared)
	{
		return;
	}

	static bool logged_render = false;
	if (!logged_render)
	{
		LOGI("clci render called");
		logged_render = true;
	}

	//clci todo add more time based logic to update it in order to simulate a screen rotation 
	//pre_rotate = !pre_rotate;

	// Before acquiring an image, as the swapchain may be recreated
	handle_rotation();

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void AttachmentTransformApi::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		drawer.checkbox("Pre-rotate with attachment transform", &pre_rotate);
		const VkExtent2D &extent = get_render_context().get_swapchain().get_extent();
		drawer.text("Transform: %s", vkb::to_string(get_render_context().get_swapchain().get_transform()).c_str());
		drawer.text("Swapchain: %u x %u", extent.width, extent.height);
	}
}

std::unique_ptr<vkb::VulkanSampleC> create_attachment_transform_api()
{
	return std::make_unique<AttachmentTransformApi>();
}
