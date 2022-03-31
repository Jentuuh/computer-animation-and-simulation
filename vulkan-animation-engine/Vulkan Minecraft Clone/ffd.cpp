#include "ffd.hpp"
#include <iostream>
#include "vmc_game_object.hpp"

namespace vmc {

	FFD::FFD()
	{
		P0 = { .0f, .0f, .0f };
		S = { 1.0f, .0f, .0f };
		U = { .0f, -1.0f, .0f };
		T = { .0f, .0f, 1.0f };
	}

	FFD::FFD(FFDInitializer init) : l{int(init.resX)}, m{int(init.resY)}, n{int(init.resZ)}
	{
		// Construct local grid space + grid basis
		P0 = { init.startX, init.startY, init.startZ };
		S = glm::vec3{ init.endX, init.startY, init.startZ } - glm::vec3{ init.startX, init.startY, init.startZ };
		T = glm::vec3{ init.startX, init.endY, init.startZ } - glm::vec3{ init.startX, init.startY, init.startZ };
		U = glm::vec3{ init.startX, init.startY, init.endZ } - glm::vec3{ init.startX, init.startY, init.startZ };
		

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

	void FFD::resetControlPoints()
	{
		// Generate control points
		TransformComponent transform{};
		transform.scale = { 0.02f, 0.02f, 0.02f };
		for (float i = 0; i <= l; i++)
		{
			for (float j = 0; j <= m; j++)
			{
				for (float k = 0; k <= n; k++)
				{
					transform.translation = P0 + (i / l) * S + (j / m) * U + (k / n) * T;
					grid[i * (l + 1) * (m + 1) + j * (m + 1) + k] = transform;
				}
			}
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

	glm::vec3 FFD::calcDeformedGlobalPosition(glm::vec3 oldPosition)
	{
		// Calculate s,t,u
		float s = glm::dot(glm::cross(T, U), oldPosition - P0) / (glm::dot(glm::cross(T, U), S));
		float t = glm::dot(glm::cross(U, S), oldPosition - P0) / (glm::dot(glm::cross(U, S), T));
		float u = glm::dot(glm::cross(S, T), oldPosition - P0) / (glm::dot(glm::cross(S, T), U));

		//std::cout << "s: " << s << std::endl;
		//std::cout << "t: " << t << std::endl;
		//std::cout << "u: " << u << std::endl;

		// Sederberg (trivariate Bezier interpolating function) (mistake is probably somewhere here, maybe because I don't convert back from S,T,U space to X,Y,Z globals?)
		glm::vec3 newPos_stu = { .0f, .0f, .0f };
		for (int i = 0; i <= l; i++)
		{
			glm::vec3 sumM = { .0f, .0f, .0f };
			for (int j = 0; j <= m; j++)
			{
				glm::vec3 sumN = { .0f, .0f, .0f };
				for (int k = 0; k <= n; k++)
				{

					float s_cp = glm::dot(glm::cross(T, U), grid[i * (l + 1) * (m + 1) + j * (m + 1) + k].translation - P0) / (glm::dot(glm::cross(T, U), S));
					float t_cp = glm::dot(glm::cross(U, S), grid[i * (l + 1) * (m + 1) + j * (m + 1) + k].translation - P0) / (glm::dot(glm::cross(U, S), T));
					float u_cp = glm::dot(glm::cross(S, T), grid[i * (l + 1) * (m + 1) + j * (m + 1) + k].translation - P0) / (glm::dot(glm::cross(S, T), U));
					glm::vec3 P_ijk = { s_cp, t_cp, u_cp };

					sumN += combinations(n, k) * powf(1 - u, n - k) * powf(u, k) * P_ijk;
				}
				sumM += combinations(m, j) * powf(1 - t, m - j) * powf(t, j) * sumN;
			}
			newPos_stu += combinations(l, i) * powf(1 - s, l - i) * powf(s, i) * sumM;
		}
		//std::cout << "s_after: " << newPos_stu.x << std::endl;
		//std::cout << "t_after: " << newPos_stu.y << std::endl;
		//std::cout << "u_after: " << newPos_stu.z << std::endl;

		glm::vec3 newPos_global = P0 + newPos_stu.x * S + newPos_stu.y * T + newPos_stu.z * U;

		return newPos_global;
	}

	void FFD::translate(glm::vec3 transVec)
	{
		P0 = P0 + transVec;
		for (auto& p : grid)
		{
			p.translation += transVec;
		}
	}


	int FFD::combinations(int n, int r)
	{
		return fact(n) / (fact(r) * fact(n - r));
	}

	// Returns factorial of n
	int FFD::fact(int n)
	{
		int res = 1;
		for (int i = 2; i <= n; i++)
			res = res * i;
		return res;
	}

}