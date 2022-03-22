#pragma once
#include "vmc_game_object.hpp"
#include "enums.hpp"

// std
#include <vector>

// lib
#include <glm/glm.hpp>

namespace vmc {

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


	class FFD
	{
	public:
		FFD(FFDInitializer init);
		std::vector<TransformComponent> getControlPoints(){ return grid; };
		int getCurrentCPIndex() { return selectedControlPoint; };
		void moveCurrentControlPoint(MoveDirection dir, float dt);
		void selectNextControlPoint();
		void selectPrevControlPoint();


	private:
		std::vector<TransformComponent> grid;
		glm::vec3 S;
		glm::vec3 U;
		glm::vec3 T;
		glm::vec3 P0;

		int selectedControlPoint = 0;
		float pointMovementSpeed = 5.0f;
	};
}
