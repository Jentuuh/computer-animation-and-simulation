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

namespace vae {

	SimpleRenderSystem::SimpleRenderSystem(VmcDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : vmcDevice{device}
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
		vkDestroyPipelineLayout(vmcDevice.device(), pipelineLayout, nullptr);
	}

	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(TestPushConstant);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };


		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
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
	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, std::vector<VmcGameObject> &gameObjects, Animator& animator, LSystem& lsystem, Skeleton& skeleton, RigidBody& rigid, const VmcCamera& camera, const float frameDeltaTime)
	{
		vmcPipeline->bind(commandBuffer);

		// Global descriptor set (index 0), can be reused by all game objects
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1,
			&globalDescriptorSet, 0,
			nullptr);

		// Advance time in animator and calculate the new position based on the current time
		animator.advanceTime(frameDeltaTime);
		glm::vec3 nextPosition = animator.calculateNextPositionSpeedControlled();
		std::shared_ptr<VmcModel> pointModel = animator.getControlPoints()[0].model;

		// Draw gameobjects
		for (auto& obj : gameObjects) {
			
			// Translate object with id 0 speed-controlled over a space curve
			if (obj.getId() == 0)
				obj.setPosition(nextPosition);

			TestPushConstant push{};
			push.modelMatrix = obj.transform.mat4();
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

				TestPushConstant pushChild{};
				pushChild.modelMatrix = child.transform.mat4();
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
				pushFFD.modelMatrix = ffdControlPoint.mat4();
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
			pushSpline.modelMatrix = cpspline.transform.mat4();
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
			pushSpline.modelMatrix = curvePoint.mat4();
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
			pushL.modelMatrix = lrenderpoint.mat4();
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

		// Draw skeleton
		//skeleton.drawSkeleton(commandBuffer, pipelineLayout, skeleton.getRoot(), projectionView);

		// Draw rigid body
		TestPushConstant pushRigid{};

		pushRigid.modelMatrix = rigid.S.mat4();
		pushRigid.normalMatrix = rigid.S.normalMatrix();
		pushRigid.color = { .5f, 1.0f, .5f };
		vkCmdPushConstants(commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(TestPushConstant),
			&pushRigid);

		rigid.model->bind(commandBuffer);
		rigid.model->draw(commandBuffer);	
	}
}