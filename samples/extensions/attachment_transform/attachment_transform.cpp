/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

#include "attachment_transform.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "filesystem/legacy.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "stats/stats.h"


AttachmentTransform::AttachmentTransform()
{
}

bool AttachmentTransform::prepare(const vkb::ApplicationOptions &options)
{
	if (!vkb::VulkanSampleC::prepare(options))
	{
		return false;
	}

	load_scene("scenes/bonza/Bonza.gltf");

	auto &camera_node = vkb::add_free_camera(get_scene(), "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	vkb::core::ShaderSource vert_shader("base.vert.spv");
	vkb::core::ShaderSource frag_shader("base.frag.spv");
	auto                    scene_subpass = std::make_unique<vkb::rendering::subpasses::ForwardSubpassC>(get_render_context(), std::move(vert_shader), std::move(frag_shader), get_scene(), *camera);

	auto render_pipeline = std::make_unique<vkb::rendering::RenderPipelineC>();
	render_pipeline->add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));

	get_stats().request_stats({vkb::StatIndex::frame_times});
	create_gui(*window, &get_stats());

	return true;
}

void AttachmentTransform::update(float delta_time)
{
	VulkanSample::update(delta_time);
}


std::unique_ptr<vkb::VulkanSampleC> create_attachment_transform()
{
	return std::make_unique<AttachmentTransform>();
}
