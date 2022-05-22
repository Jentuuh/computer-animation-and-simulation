#pragma once
#include "bone.hpp"
#include <glm/glm.hpp>


namespace vae {
	class Skeleton2
	{
	public:
		Skeleton2(std::shared_ptr<VmcModel> boneMod);

		void addRoot(glm::vec3 pos, float len, glm::vec3 rot);
		void addBone(float len, glm::vec3 rot);
		void render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout);
		void update();

		glm::vec3 focusPoint;
	private:
		Bone* root;
		std::vector<std::shared_ptr<Bone>> boneData;
		std::shared_ptr<VmcModel> boneModel;
	};

}
