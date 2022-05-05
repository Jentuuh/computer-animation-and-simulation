#pragma once
#include "enums.hpp"

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
		float currentTime = 0.0f;
		float animationTime = 4.0f;
	};


	class FFD
	{
	public:
		FFD();
		FFD(FFDInitializer init);

		std::vector<TransformComponent> getControlPoints(){ return grid; };
		int getAmountKeyframes() { return animationProps.keyframes.size(); };
		int getCurrentCPIndex() { return selectedControlPoint; };


		void moveCurrentControlPoint(MoveDirection dir, float dt);
		void resetControlPoints();
		void selectNextControlPoint();
		void selectPrevControlPoint();

		void addKeyFrame();
		void delKeyFrame(int index);
		void advanceTime(float dt);
		void resetTime() { animationProps.currentTime = 0.0f; };
		void interpolateControlPoints();
		void setInitialKeyFrameControlPoints();

		void translate(glm::vec3 transVec);

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

		int l;		// Resolutions per dimension
		int m;
		int n;

		int selectedControlPoint = 0;
		float pointMovementSpeed = 5.0f;
	};
}
