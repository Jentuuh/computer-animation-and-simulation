#pragma once
#include "animatable.hpp"
#include "bone.hpp"
#include <glm/glm.hpp>


namespace vae {
	class Skeleton2 : public Animatable
	{
	public:
		Skeleton2(std::shared_ptr<VmcModel> boneMod);

		std::vector<std::shared_ptr<Bone>>& getBones() { return boneData; };

		void updateAnimatable();
		void cleanUpAnimatable();
		void addRoot(glm::vec3 pos, float len, glm::vec3 rot);
		void addBone(float len, glm::vec3 rot);
		void render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout);
		void update();
		void addKeyFrame();

		glm::vec3 focusPoint;
	private:
		Bone* root;
		std::vector<std::shared_ptr<Bone>> boneData;
		std::shared_ptr<VmcModel> boneModel;
	};

}
