#pragma once

#include "vmc_camera.hpp"
#include "vmc_pipeline.hpp"
#include "vmc_device.hpp"
#include "vmc_game_object.hpp"
#include "spline_animator.hpp"
#include "l_system.hpp"
#include "ffd.hpp"
#include "skeleton.hpp"

// std 
#include <memory>
#include <vector>

namespace vae {
	class SimpleRenderSystem
	{
	public:

		SimpleRenderSystem(VmcDevice& device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VmcGameObject> &gameObjects, Animator& animator, LSystem& lsystem, Skeleton& skeleton,const VmcCamera& camera, const float frameDeltaTime);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);

		VmcDevice& vmcDevice;

		float clock;
		std::unique_ptr<VmcPipeline> vmcPipeline;
		VkPipelineLayout pipelineLayout;
	};
}
