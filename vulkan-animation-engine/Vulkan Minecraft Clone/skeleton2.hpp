#pragma once
#include "animatable.hpp"
#include "bone.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>


namespace vae {
	enum KinematicsMode {
		FORWARD,
		INVERSE
	};
	class Skeleton2 : public Animatable
	{
	public:
		Skeleton2(std::shared_ptr<VmcModel> boneMod);

		std::vector<std::shared_ptr<Bone>>& getBones() { return boneData; };

		void updateAnimatable();
		void cleanUpAnimatable();
		void addKeyFrameFK();
		void addKeyFrameIK();

		void addRoot(glm::vec3 pos, float len, glm::vec3 rot);
		void addBone(float len, glm::vec3 rot);

		void render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> pointModel);
		
		std::vector<glm::vec3> FK();
		void solveIK_Z(int maxIterations= 1000, float errorMin=0.1f);

		glm::vec3 focusPoint;
		bool drawIKTarget = true;
		int mode = FORWARD;

	private:
		Bone* root;
		std::vector<std::shared_ptr<Bone>> boneData;
		std::shared_ptr<VmcModel> boneModel;
		std::vector<glm::vec3> IK_Keyframes;
	};

}
