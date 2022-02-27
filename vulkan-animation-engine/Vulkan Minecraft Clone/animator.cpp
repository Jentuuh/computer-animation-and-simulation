#include "animator.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include <algorithm>
#include <iostream>

namespace vmc {
	Animator::Animator()
	{
		current_cp = 0;
		pathProgress = 0.0f;
		movementSpeed = 0.1f;
	}

	void Animator::addControlPoint(glm::vec3 pos, std::shared_ptr<VmcModel> model)
	{
		auto contr_point = VmcGameObject::createGameObject();
		contr_point.model = model;
		contr_point.transform.translation = pos;
		contr_point.transform.scale = { 0.05f, 0.05f, 0.05f };
		contr_point.color = { 1.0f, 0.0f, 0.0f };

		controlPoints.push_back(std::move(contr_point));
	}

	void Animator::advanceToNextControlPoint()
	{
		current_cp = (current_cp + 1) % controlPoints.size();
	}

	glm::vec3 Animator::calculateNextPosition(float deltaTime)
	{
		pathProgress += deltaTime * movementSpeed;
		std::clamp<float>(pathProgress, 0.0f, 1.0f);
		glm::vec3 newPos = controlPoints[current_cp].transform.translation * pathProgress + controlPoints[current_cp + 1].transform.translation * (1 - pathProgress);
		if (pathProgress > 0.99f) {
			advanceToNextControlPoint();
			std::cout << glm::to_string(controlPoints[current_cp].transform.translation) << std::endl;
			pathProgress = 0.0f;
		}
		return newPos;
	}

}