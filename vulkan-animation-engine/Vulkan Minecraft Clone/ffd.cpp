#include "ffd.hpp"
#include <iostream>

namespace vmc {

	FFD::FFD(FFDInitializer init)
	{
		// Construct local grid space + grid basis
		P0 = { init.startX, init.startY, init.startZ };
		S = glm::vec3{ init.startX, init.startY, init.startZ } - glm::vec3{ init.endX, init.startY, init.startZ };
		U = glm::vec3{ init.startX, init.startY, init.startZ } - glm::vec3{ init.startX, init.startY, init.endZ };
		T = glm::vec3{ init.startX, init.startY, init.startZ } - glm::vec3{ init.startX, init.endY, init.startZ };

		// Generate control points
		TransformComponent transform{};
		transform.scale = { 0.02f, 0.02f, 0.02f };
		for (float i = 0; i <= init.resX; i++) 
		{
			for (float j = 0; j <= init.resY; j++)
			{
				for (float k = 0; k <= init.resZ; k++)
				{
					transform.translation = P0 + (i / init.resX) * S + (j / init.resY) * U + (k / init.resZ) * T;
					grid.push_back(transform);
				}
			}
		}
	}

	void FFD::moveCurrentControlPoint(MoveDirection dir, float dt)
	{
		switch (dir)
		{
		case POSX:
			grid[selectedControlPoint].translation.x += dt * pointMovementSpeed;
			break;
		case NEGX:
			grid[selectedControlPoint].translation.x -= dt * pointMovementSpeed;
			break;
		case POSY:
			grid[selectedControlPoint].translation.y -= dt * pointMovementSpeed;
			break;
		case NEGY:
			grid[selectedControlPoint].translation.y += dt * pointMovementSpeed;
			break;
		case POSZ:
			grid[selectedControlPoint].translation.z += dt * pointMovementSpeed;
			break;
		case NEGZ:
			grid[selectedControlPoint].translation.z -= dt * pointMovementSpeed;
			break;

		default:
			break;
		}
	}

	void FFD::selectNextControlPoint()
	{
		selectedControlPoint = (selectedControlPoint + 1) % grid.size();
	}

	void FFD::selectPrevControlPoint()
	{
		selectedControlPoint = (selectedControlPoint - 1) % grid.size();
	}
}