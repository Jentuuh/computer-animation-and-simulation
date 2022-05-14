#include "ffd.hpp"
#include <iostream>
#include "vmc_game_object.hpp"
#include "simple_render_system.hpp"


namespace vae {

	FFD::FFD(): Animatable(0.0f, 4.0f)
	{
		P0 = { .0f, .0f, .0f };
		S = { 1.0f, .0f, .0f };
		U = { .0f, -1.0f, .0f };
		T = { .0f, .0f, 1.0f };
	}

	FFD::FFD(FFDInitializer init) : Animatable(0.0f, 4.0f), l{int(init.resX)}, m{int(init.resY)}, n{int(init.resZ)}
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

	void FFD::updateAnimatable()
	{
		interpolateControlPoints();
	}


	void FFD::updateTransformation(glm::mat4 newTransformation)
	{
		transformation = newTransformation;
	}

	void FFD::render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> pointModel)
	{
		int idx = 0;
		TestPushConstant pushFFD{};
		for (auto& ffdControlPoint : grid)
		{
			pushFFD.modelMatrix = transformation * ffdControlPoint.mat4();
			pushFFD.normalMatrix = ffdControlPoint.normalMatrix();
			if (idx == getCurrentCPIndex())
			{
				pushFFD.color = { 1.0f, 1.0f, 1.0f };
			}
			else {
				pushFFD.color = { .0f, 1.0f, 1.0f };
			}

			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushFFD);

			pointModel->bind(commandBuffer);
			pointModel->draw(commandBuffer);
			idx++;
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
		// Reset control points to initial position
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

	void FFD::addKeyFrame()
	{
		std::vector<glm::vec3> newKeyFrame{};
		for (auto& point : grid)
		{
			newKeyFrame.push_back(point.translation);
		}
		animationProps.keyframes.push_back(newKeyFrame);
	}

	void FFD::delKeyFrame(int index)
	{
		animationProps.keyframes.erase(animationProps.keyframes.begin() + index);
	}

	void FFD::interpolateControlPoints()
	{
		float normalizedTimePassed = timePassed / duration;
		float fractionPerKeyFrame = 1.0f / static_cast<float>(animationProps.keyframes.size() - 1);

		float index = normalizedTimePassed / fractionPerKeyFrame;
		int roundedIndex = floor(index);
		float keyFrameProgress = index - static_cast<float>(roundedIndex);

		if (roundedIndex < animationProps.keyframes.size() - 1)
		{
			std::vector<glm::vec3> prev_keyframe = animationProps.keyframes[roundedIndex];
			std::vector<glm::vec3> next_keyframe = animationProps.keyframes[roundedIndex + 1];

			for (int i = 0; i < prev_keyframe.size(); i++)
			{
				// Linear interpolation between keyframes
				grid[i].translation = prev_keyframe[i] + keyFrameProgress * (next_keyframe[i] - prev_keyframe[i]);
			}
		}
	}

	// Set control points to the layout of the initial keyframe. 
	// This is necessary at the start (or each repetition of) the deformation animation.
	void FFD::setInitialKeyFrameControlPoints()
	{
		for (int i = 0; i < animationProps.keyframes[0].size(); i++)
		{
			grid[i].translation = animationProps.keyframes[0][i];
		}
	}


	glm::vec3 FFD::calcDeformedGlobalPosition(glm::vec3 oldPosition)
	{
		// Calculate s,t,u
		float s = glm::dot(glm::cross(T, U), oldPosition - P0) / (glm::dot(glm::cross(T, U), S));
		float t = glm::dot(glm::cross(U, S), oldPosition - P0) / (glm::dot(glm::cross(U, S), T));
		float u = glm::dot(glm::cross(S, T), oldPosition - P0) / (glm::dot(glm::cross(S, T), U));

		// Sederberg (trivariate Bezier interpolating function)
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

		glm::vec3 newPos_global = P0 + newPos_stu.x * S + newPos_stu.y * T + newPos_stu.z * U;

		return newPos_global;
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