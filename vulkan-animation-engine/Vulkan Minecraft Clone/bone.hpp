#pragma once
#include "vmc_model.hpp"

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

// std
#include <memory>
#include <vector>

namespace vae {
	class Bone
	{
	public:
		Bone(Bone* parent, float len, glm::vec3 rot);
		Bone(glm::vec3 pos, float len, glm::vec3 rot);

		glm::mat4 getTransform() { return globalTransformationMatrix; };
		glm::vec3& getRotation() { return rotation; };
		Bone* getChild() { return child_; };
		std::vector<glm::vec3>& getKeyFrames() { return keyframes; };

		void render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> boneModel);
		void updateAnimatable(float kfIndex, float kfFraction);
		void updateRotation();
		void setChild(Bone* child);
		void follow(glm::vec3 target);
		void addKeyFrame();

	private:
		Bone* parent_;
		Bone* child_;

		glm::vec3 posOffset;
		glm::mat4 localTransformation;
		glm::mat4 globalTransformationMatrix;
		glm::vec3 rotation;
		float length;

		std::vector<glm::vec3> keyframes;
	};
}


