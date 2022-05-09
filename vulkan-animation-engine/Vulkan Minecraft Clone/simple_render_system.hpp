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

		SimpleRenderSystem(VmcDevice& device, VkRenderPass sceneRenderPass, VkRenderPass skyboxRenderPass, VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		bool& shouldRenderSkybox() { return renderSkybox; };
		void renderGameObjects(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, VkDescriptorSet skyboxDescriptorSet, std::vector<VmcGameObject>& skyBoxes,
								std::vector<VmcGameObject> &gameObjects, std::vector<SplineAnimator>& animators, 
								LSystem& lsystem, Skeleton& skeleton, std::vector<RigidBody>& rigids, std::vector<RigidBody>& collidables, const VmcCamera& camera,
								const float frameDeltaTime, std::shared_ptr<VmcModel> pointModel, int camMode, VmcGameObject* viewerObj);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);
		void createSkyBoxPipeline(VkRenderPass renderPass);

		VmcDevice& vmcDevice;

		bool renderSkybox = true;
		float clock;
		std::unique_ptr<VmcPipeline> vmcPipeline;
		std::unique_ptr<VmcPipeline> skyboxPipeline;
		VkPipelineLayout pipelineLayout;
	};
}
