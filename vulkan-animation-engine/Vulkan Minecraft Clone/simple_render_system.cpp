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

	SimpleRenderSystem::SimpleRenderSystem(VmcDevice &device, VkRenderPass sceneRenderPass,  VkRenderPass skyboxRenderPass, VkDescriptorSetLayout globalSetLayout) : vmcDevice{device}
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(sceneRenderPass);
		createSkyBoxPipeline(skyboxRenderPass);

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

	void SimpleRenderSystem::createSkyBoxPipeline(VkRenderPass renderPass)
	{
		assert(skyboxPipelineLayout != nullptr && "Pipeline layout should be created before pipeline creation!");

		PipelineConfigInfo pipelineConfig{};
		VmcPipeline::skyboxPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		skyboxPipeline = std::make_unique<VmcPipeline>(vmcDevice, "../Shaders/skybox_shader.vert.spv", "../Shaders/skybox_shader.frag.spv", pipelineConfig);
	}


	// TODO: State update of objects should be handled somewhere else!
	// Render loop
	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, VkDescriptorSet skyboxDescriptorSet, std::vector<VmcGameObject>& skyBoxes, std::vector<VmcGameObject> &gameObjects, std::vector<SplineAnimator>& animators, std::vector<LSystem>& lsystems, Skeleton& skeleton, std::vector<RigidBody>& rigids, std::vector<RigidBody>& collidables, const VmcCamera& camera, const float frameDeltaTime, std::shared_ptr<VmcModel> pointModel, int camMode, VmcGameObject* viewerObj)
	{
		if (renderSkybox)
		{
			// ============
			// Draw skybox
			// ============
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0, 1,
				&skyboxDescriptorSet, 0,
				nullptr);

			skyBoxes[0].model->bind(commandBuffer);
			skyboxPipeline->bind(commandBuffer);
			skyBoxes[0].model->draw(commandBuffer);
		}

		// ===========
		// Draw scene
		// ===========
		vmcPipeline->bind(commandBuffer);
		// Global descriptor set (index 0), can be reused by all game objects
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1,
			&globalDescriptorSet, 0,
			nullptr);

		glm::vec3 nextPosition{ 0.0f, 0.0f, 0.0f };
		glm::vec3 nextRotation{ 0.0f, 0.0f, 0.0f };

		for (int i = 0; i < animators.size(); i++)
		{
			// Draw spline control points
			TestPushConstant pushSpline{};
			for (auto& cpspline : animators[i].getControlPoints())
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
			if (camMode != 2)
			{
				pushSpline.color = { 1.0f, 1.0f, 1.0f };
				for (auto& curvePoint : animators[i].getCurvePoints())
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
			}
		}

		// Draw gameobjects
		for (auto& obj : gameObjects) {

			// Update object deformation
			if (obj.deformationEnabled)
			{
				obj.deformObject();
			}

			TestPushConstant push{};
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix();
			push.color = obj.color;
			
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

				TestPushConstant pushChild{};
				pushChild.modelMatrix = child.transform.mat4();
				pushChild.normalMatrix = child.transform.normalMatrix();
				pushChild.color = obj.color;

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
			obj.deformationSystem.render(commandBuffer, pipelineLayout, pointModel);
		}


		// Draw L-Systems
		TestPushConstant pushL{};
		for (auto& lsystem : lsystems)
		{
			for (auto& lrenderpoint : lsystem.getRenderPoints())
			{
				pushL.modelMatrix = lrenderpoint.mat4();
				pushL.normalMatrix = lrenderpoint.normalMatrix();
				pushL.color = lsystem.renderColor;

				vkCmdPushConstants(commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					0,
					sizeof(TestPushConstant),
					&pushL);

				pointModel->bind(commandBuffer);
				pointModel->draw(commandBuffer);
			}
		}


		// Draw skeleton
		//skeleton.drawSkeleton(commandBuffer, pipelineLayout, skeleton.getRoot(), projectionView);

		// Draw rigid bodies
		TestPushConstant pushRigid{};

		for (auto& rigid : rigids)
		{
			pushRigid.modelMatrix = rigid.S.mat4();
			pushRigid.normalMatrix = rigid.S.normalMatrix();
			pushRigid.color = { 0.17f, 0.73f, 0.84f };
			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushRigid);

			rigid.model->bind(commandBuffer);
			rigid.model->draw(commandBuffer);
		}

		// Draw collidables
		TestPushConstant pushCol{};

		for (auto& col : collidables)
		{
			pushCol.modelMatrix = col.S.mat4();
			pushCol.normalMatrix = col.S.normalMatrix();
			pushCol.color = { 0.04f, 0.22f, 0.08f };
			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushCol);

			col.model->bind(commandBuffer);
			col.model->draw(commandBuffer);
		}
	}
}