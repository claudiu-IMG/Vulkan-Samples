/* Copyright (c) 2024, Arm Limited and Contributors
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

#pragma once

#include "api_vulkan_sample.h"

class AttachmentTransform : public ApiVulkanSample
{
  public:
	AttachmentTransform();
	~AttachmentTransform() override;

	bool prepare(const vkb::ApplicationOptions &options) override;
	void request_device_extensions(std::unordered_map<std::string, vkb::RequestMode> &requested_extensions) const override;
	void request_gpu_features(vkb::core::PhysicalDeviceC &gpu) override;
	void setup_render_pass() override;
	void setup_framebuffer() override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
	void create_pipeline();
	void handle_rotation();

	bool             pre_rotate       = true;
	int8_t    		 rotate_index     = 0;
	uint32_t 	     frame_counter    = 0;
	VkPipelineLayout pipeline_layout  = VK_NULL_HANDLE;
	VkPipeline       pipeline         = VK_NULL_HANDLE;
};


std::unique_ptr<vkb::VulkanSampleC> create_attachment_transform();
