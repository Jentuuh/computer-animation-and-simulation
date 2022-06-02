#pragma once
#include "enums.hpp"
#include "vmc_model.hpp"
#include "animatable.hpp"

// std
#include <vector>

// lib
#include <glm/glm.hpp>

namespace vae {
	struct TransformComponent;

	struct FFDInitializer {
		// Grid dimensions
		float startX; 
		float endX; 
		float startY;
		float endY;
		float startZ;
		float endZ;

		// Resolutions
		float resX;
		float resY;
		float resZ;
	};

	struct AnimationProperties
	{
		std::vector<std::vector<glm::vec3>> keyframes{};
	};


	class FFD : public Animatable
	{
	public:
		FFD();
		FFD(FFDInitializer init);

		void updateAnimatable();
		void cleanUpAnimatable();
		std::vector<TransformComponent> getControlPoints(){ return grid; };
		int getAmountKeyframes() { return animationProps.keyframes.size(); };
		std::vector<std::vector<glm::vec3>>& getKeyFrames() { return animationProps.keyframes; };
		int getCurrentCPIndex() { return selectedControlPoint; };
		void updateTransformation(glm::mat4 newTransformation);

		void render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> pointModel);
		void moveCurrentControlPoint(MoveDirection dir, float dt);
		void resetControlPoints();
		void selectNextControlPoint();
		void selectPrevControlPoint();

		void addKeyFrame();
		void addKeyFrame(std::vector<glm::vec3> CPs);
		void delKeyFrame(int index);
		void resetTime() { timePassed = 0.0f; };
		void interpolateControlPoints();
		void setInitialKeyFrameControlPoints();

		glm::vec3 calcDeformedGlobalPosition(glm::vec3 oldPosition);

		bool resetModel = false;
		AnimationProperties animationProps;
	private:
		int fact(int n);
		int combinations(int n, int r);

		std::vector<TransformComponent> grid;
		glm::vec3 S;
		glm::vec3 U;
		glm::vec3 T;
		glm::vec3 P0;
		glm::mat4 transformation{1.0f};

		int l;		// Resolutions per dimension
		int m;
		int n;

		int selectedControlPoint = 0;
		float pointMovementSpeed = 5.0f;
	};
}
