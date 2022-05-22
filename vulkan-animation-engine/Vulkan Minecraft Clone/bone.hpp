#pragma once
#include "vmc_model.hpp"

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

// std
#include <memory>

namespace vae {
	class Bone
	{
	public:
		Bone(Bone* parent, float len, glm::vec3 rot);
		Bone(glm::vec3 pos, float len, glm::vec3 rot);

		glm::mat4 getTransform() { return transformationMatrix; };
		Bone* getChild() { return child_; };

		void render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> boneModel);
		void setChild(Bone* child);
		void follow(glm::vec3 target);

	private:
		Bone* parent_;
		Bone* child_;

		glm::vec3 posOffset;
		glm::mat4 localTransform;
		glm::mat4 transformationMatrix;
		glm::vec3 rotation;
		float length;
	};
}


