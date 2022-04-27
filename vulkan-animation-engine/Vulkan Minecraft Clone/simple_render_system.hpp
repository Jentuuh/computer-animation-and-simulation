#pragma once

#include "vmc_camera.hpp"
#include "vmc_pipeline.hpp"
#include "vmc_device.hpp"
#include "vmc_game_object.hpp"
#include "spline_animator.hpp"
#include "l_system.hpp"
#include "ffd.hpp"
#include "skeleton.hpp"
#include "rigid_body.hpp"

// std 
#include <memory>
#include <vector>

namespace vae {
	struct TestPushConstant {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
		glm::vec3 color{ 1.f };
	};

	class SimpleRenderSystem
	{
	public:

		SimpleRenderSystem(VmcDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, std::vector<VmcGameObject> &gameObjects, Animator& animator, LSystem& lsystem, Skeleton& skeleton, RigidBody& rigid, const VmcCamera& camera, const float frameDeltaTime);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		VmcDevice& vmcDevice;

		float clock;
		std::unique_ptr<VmcPipeline> vmcPipeline;
		VkPipelineLayout pipelineLayout;
	};
}
