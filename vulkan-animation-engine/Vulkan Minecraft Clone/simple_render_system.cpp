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
	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VmcGameObject> &gameObjects, Animator& animator, LSystem& lsystem, const VmcCamera& camera, const float frameDeltaTime)
	{
		vmcPipeline->bind(commandBuffer);

		auto projectionView = camera.getProjection() * camera.getView();
		
		// Update clock (for periodical behaviour)
		/*clock = fmod(clock + frameDeltaTime, 2);*/

		// Advance time in animator and calculate the new position based on the current time
		animator.advanceTime(frameDeltaTime);
		glm::vec3 nextPosition = animator.calculateNextPositionSpeedControlled();
		std::shared_ptr<VmcModel> pointModel = animator.getControlPoints()[0].model;

		// Draw gameobjects
		for (auto& obj : gameObjects) {
			
			// Translate object with id 0 speed-controlled over a space curve
		/*	if (obj.getId() == 0)
				obj.setPosition(nextPosition);*/

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

			// Draw children
			for (auto& child : obj.getChildren()) {

				// Rotate arm
				child.setPosition(nextPosition);
				child.transform.rotation = animator.calculateNextRotationParabolic();
				auto childModelMatrix = child.transform.mat4();

				TestPushConstant pushChild{};
				pushChild.transform = projectionView * childModelMatrix;
				pushChild.normalMatrix = child.transform.normalMatrix();
				pushChild.color = { 0.f, 1.f, 0.f };

				vkCmdPushConstants(
					commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					0,
					sizeof(TestPushConstant),
					&pushChild);

				child.model->bind(commandBuffer);
				child.model->draw(commandBuffer);
			}

			// Draw deformation grid
			int idx = 0;
			TestPushConstant pushFFD{};
			for (auto& ffdControlPoint : obj.deformationSystem.getControlPoints())
			{
				auto modelMatrix = ffdControlPoint.mat4();
				pushFFD.transform = projectionView * modelMatrix;
				pushFFD.normalMatrix = ffdControlPoint.normalMatrix();
				if (idx == obj.deformationSystem.getCurrentCPIndex())
				{
					pushFFD.color = { 1.0f, 1.0f, 1.0f };
				}
				else {
					pushFFD.color = { .0f, 1.0f, 1.0f };
				}

				vkCmdPushConstants(commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					0,
					sizeof(TestPushConstant),
					&pushFFD);

				pointModel->bind(commandBuffer);
				pointModel->draw(commandBuffer);
				idx++;
			}
		}

		TestPushConstant pushSpline{};

		// Draw spline control points
		for (auto& cpspline : animator.getControlPoints())
		{
			auto modelMatrix = cpspline.transform.mat4();
			pushSpline.transform = projectionView * modelMatrix;
			pushSpline.normalMatrix = cpspline.transform.normalMatrix();
			pushSpline.color = cpspline.color;

			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushSpline);

			cpspline.model->bind(commandBuffer);
			cpspline.model->draw(commandBuffer);
		}

		// Draw spline curve points
		pushSpline.color = { 1.0f, 1.0f, 1.0f };
		for (auto& curvePoint : animator.getCurvePoints())
		{
			auto modelMatrix = curvePoint.mat4();
			pushSpline.transform = projectionView * modelMatrix;
			pushSpline.normalMatrix = curvePoint.normalMatrix();
			pushSpline.color = { 1.0f, 1.0f, 1.0f };

			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushSpline);

			pointModel->bind(commandBuffer);
			pointModel->draw(commandBuffer);
		}

		// Draw L-System
		TestPushConstant pushL{};

		for (auto& lrenderpoint : lsystem.getRenderPoints())
		{
			auto modelMatrix = lrenderpoint.mat4();
			pushL.transform = projectionView * modelMatrix;
			pushL.normalMatrix = lrenderpoint.normalMatrix();
			pushL.color = {1.0f, 1.0f, .0f};

			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushL);

			pointModel->bind(commandBuffer);
			pointModel->draw(commandBuffer);
		}

		// Draw Deformation Grid
		/*int idx = 0;
		for (auto& ffdControlPoint : deformationSystem.getControlPoints())
		{
			auto modelMatrix = ffdControlPoint.mat4();
			pushL.transform = projectionView * modelMatrix;
			pushL.normalMatrix = ffdControlPoint.normalMatrix();
			if (idx == deformationSystem.getCurrentCPIndex())
			{
				pushL.color = { 1.0f, 1.0f, 1.0f };
			}
			else {
				pushL.color = { .0f, 1.0f, 1.0f };
			}

			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushL);

			pointModel->bind(commandBuffer);
			pointModel->draw(commandBuffer);
			idx++;
		}*/
	}
}