#include "bone.hpp"
#include "simple_render_system.hpp"
#include <iostream>
#include <glm/gtx/string_cast.hpp>

namespace vae {
	// Child constructor
	Bone::Bone(Bone* parent, float len, glm::vec3 rot): rotation{ rot }, length{ len }
	{
		parent_ = parent;
		child_ = nullptr;

		// Local transform (first rotate, then translate)
		glm::mat4 sx = glm::scale(glm::mat4(1.0f), { len, 1.0f, 1.0f });
		glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f });
		glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f });
		glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f });
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), { 1.0f, 0.0f, 0.0f });
		localTransformation = trans * rx * ry * rz;
		
		// Global transform
		globalTransformationMatrix = parent_->globalTransformationMatrix * localTransformation;
		//posOffset = glm::vec3(globalTransformationMatrix[3].x, globalTransformationMatrix[3].y, globalTransformationMatrix[3].z);
	}

	// Root constructor
	Bone::Bone(glm::vec3 pos, float len, glm::vec3 rot): posOffset{pos}, rotation{rot}, length{len}
	{
		parent_ = nullptr;
		child_ = nullptr;

		glm::mat4 sx = glm::scale(glm::mat4(1.0f), { len, 1.0f, 1.0f });
		glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f });
		glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f });
		glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f });
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), pos);
		globalTransformationMatrix = trans * rx * ry * rz;
		localTransformation = trans * rx * ry * rz;
	}


	void Bone::render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> boneModel)
	{
		TestPushConstant pushBone{};
		pushBone.modelMatrix = globalTransformationMatrix;
		pushBone.normalMatrix = glm::mat4(1.0f);
		pushBone.color = { 1.0f, 1.0f, 1.0f };
		
		vkCmdPushConstants(commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(TestPushConstant),
			&pushBone);

		boneModel->bind(commandBuffer);
		boneModel->draw(commandBuffer);
	}

	void Bone::updateAnimatable(float kfIndex, float kfFraction)
	{
		glm::vec3 interpolatedRotation = keyframes[(int)kfIndex] + kfFraction * (keyframes[(int)kfIndex + 1] - keyframes[(int)kfIndex]);
		rotation = interpolatedRotation;
		updateRotation();
	}


	void Bone::updateRotation()
	{
		glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f });
		glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f });
		glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f });
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), { 1.0f, 0.0f, 0.0f });
		localTransformation = trans * rx * ry * rz;

		if (parent_ != nullptr)
			globalTransformationMatrix = parent_->globalTransformationMatrix * localTransformation;
		else {
			globalTransformationMatrix = localTransformation;
		}
		if (child_ != nullptr)
			child_->updateRotation();
	}


	void Bone::setChild(Bone* child)
	{
		child_ = child;
	}


	void Bone::follow(glm::vec3 target)
	{
		//// TODO: try to transform the target to the bone's local axis system, then calculate the rotations around axis, apply them locally together with the parents' transform
		//glm::vec3 targetPosRelativeToBone = glm::inverse(transformationMatrix) * glm::vec4{ target, 1.0f };
		//glm::vec3 viewDir = targetPosRelativeToBone - glm::vec3{ 0.0f, 0.0f, 0.0f };
		//glm::vec3 right = glm::cross(viewDir, glm::vec3{ 0.0, -1.0, 0.0 });
		//glm::vec3 up = glm::cross(viewDir, right);

		//glm::mat4 rotMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 0.0f }, targetPosRelativeToBone, up);
		//glm::mat4 trans = glm::translate(glm::mat4(1.0f), bonePosition);
		//transformationMatrix = trans * rotMatrix;
		// 
		// Extract translation from transformation matrix
		glm::vec3 bonePosition = glm::vec3(globalTransformationMatrix[3].x, globalTransformationMatrix[3].y, globalTransformationMatrix[3].z);
		//std::cout << "Bone pos:" << glm::to_string(bonePosition) << std::endl;

		glm::vec3 boneDirection = target - bonePosition;

		glm::vec3 xy_proj = { boneDirection.x, boneDirection.y, 0.0f };
		glm::vec3 yz_proj = { 0.0f, boneDirection.y, boneDirection.z };
		glm::vec3 xz_proj = { boneDirection.x, 0.0f, boneDirection.z };

		float x_angle = glm::atan(yz_proj.y, yz_proj.z);
		float y_angle = glm::atan(xz_proj.z, xz_proj.x);
		float z_angle = glm::atan(xy_proj.x, xy_proj.y);
		//std::cout << "X:" << glm::degrees(x_angle) << std::endl;
		//std::cout << "Y:" << glm::degrees(y_angle) << std::endl;
		//std::cout << "Z:" << glm::degrees(z_angle) << std::endl;

		// Update transformation matrix
		glm::mat4 rx = glm::rotate(glm::mat4(1.0f), x_angle, { 1.0f, 0.0f, 0.0f });
		glm::mat4 ry = glm::rotate(glm::mat4(1.0f), y_angle, { 0.0f, 1.0f, 0.0f });
		glm::mat4 rz = glm::rotate(glm::mat4(1.0f), z_angle, { 0.0f, 0.0f, 1.0f });
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), bonePosition);
		globalTransformationMatrix = trans * rx * ry * rz;

	/*	if (parent_ != nullptr)
		{
			parent_->follow(bonePosition);
		}*/
	}

	void Bone::addKeyFrameFK()
	{
		keyframes.push_back(rotation);
	}
}