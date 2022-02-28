#include "simple_render_system.hpp"

// std
#include <cassert>
#include <stdexcept>
#include <array>
#include <chrono>
#include <math.h>


// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include<glm/gtc/constants.hpp>

namespace vmc {

	struct TestPushConstant {
		glm::mat4 transform{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
		glm::vec3 color{ 1.f };
	};

	SimpleRenderSystem::SimpleRenderSystem(VmcDevice &device, VkRenderPass renderPass) : vmcDevice{device}
	{
		createPipelineLayout();
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
		vkDestroyPipelineLayout(vmcDevice.device(), pipelineLayout, nullptr);
	}

	void SimpleRenderSystem::createPipelineLayout()
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(TestPushConstant);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(vmcDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw new std::runtime_error("Failed to create pipeline layout!");
		}
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass)
	{
		assert(pipelineLayout != nullptr && "Pipeline layout should be created before pipeline creation!");

		PipelineConfigInfo pipelineConfig{};
		VmcPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		vmcPipeline = std::make_unique<VmcPipeline>(vmcDevice, "../Shaders/simple_shader.vert.spv", "../Shaders/simple_shader.frag.spv", pipelineConfig);
	}

	// Render loop
	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VmcGameObject> &gameObjects, Animator& animator, const VmcCamera& camera, const float frameDeltaTime)
	{
		vmcPipeline->bind(commandBuffer);

		auto projectionView = camera.getProjection() * camera.getView();
		
		// Update clock (for periodical behaviour)
		clock = fmod(clock + frameDeltaTime, 2);
		
		// Draw gameobjects
		for (auto& obj : gameObjects) {
			// Translate object with id 0 in circles periodically
			if (obj.getId() == 0)
				obj.setPosition(animator.calculateNextPosition(frameDeltaTime));
				//obj.setPosition(glm::vec3(glm::cos(clock * glm::pi<float>()), 0.0f, glm::sin(clock * glm::pi<float>())));
			auto modelMatrix = obj.transform.mat4();

			TestPushConstant push{};
			push.transform = projectionView * modelMatrix;
			push.normalMatrix = obj.transform.normalMatrix();
			push.color = { 1.f, 1.f, 1.f };
			
			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&push);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
		
		
		// Draw control points
		for (auto& cp : animator.getControlPoints()) {
			auto modelMatrix = cp.transform.mat4();
			TestPushConstant push1{};
			push1.transform = projectionView * modelMatrix;
			push1.normalMatrix = cp.transform.normalMatrix();
			push1.color = cp.color;

			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&push1);
			cp.model->bind(commandBuffer);
			cp.model->draw(commandBuffer);
		}
	}
}