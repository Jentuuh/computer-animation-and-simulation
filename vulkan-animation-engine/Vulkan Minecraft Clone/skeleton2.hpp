#pragma once
#include "animatable.hpp"
#include "bone.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>


namespace vae {
	class Skeleton2 : public Animatable
	{
	public:
		Skeleton2(std::shared_ptr<VmcModel> boneMod);

		std::vector<std::shared_ptr<Bone>>& getBones() { return boneData; };

		void updateAnimatable();
		void cleanUpAnimatable();
		void addKeyFrame();

		void addRoot(glm::vec3 pos, float len, glm::vec3 rot);
		void addBone(float len, glm::vec3 rot);

		void render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> pointModel);
		
		std::vector<glm::vec3> FK();
		void solveIK(int maxIterations=1000, float errorMin=0.1f);

		glm::vec3 focusPoint;
		bool drawIKTarget = true;
	private:
		Bone* root;
		std::vector<std::shared_ptr<Bone>> boneData;
		std::shared_ptr<VmcModel> boneModel;
	};

}
