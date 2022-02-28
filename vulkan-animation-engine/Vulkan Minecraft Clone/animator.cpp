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
		movementSpeed = 30.0f;
	}

	void Animator::addControlPoint(glm::vec3 pos, glm::vec3 color, std::shared_ptr<VmcModel> model)
	{
		auto contr_point = VmcGameObject::createGameObject();
		contr_point.model = model;
		contr_point.transform.translation = pos;
		contr_point.transform.scale = { 0.05f, 0.05f, 0.05f };
		contr_point.color = color;

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
		glm::vec3 newPos = controlPoints[(current_cp + 1) % controlPoints.size()].transform.translation * pathProgress + controlPoints[current_cp].transform.translation * (1 - pathProgress);

		if (pathProgress > 0.99f) {
			pathProgress = 0.0f;
			advanceToNextControlPoint();
		}
		return newPos;
	}


	void Animator::buildForwardDifferencingTable()
	{
		// Calculate parametric values for each point as well as the arc lengths, incrementally
		for (int i = 0; i < controlPoints.size(); i++)
		{
			float parametric_value = (float)i / (controlPoints.size() - 1);
			float arc_length = 0.0f;
			float prev_dist;

			if (i == 0)
			{
				arc_length = 0.0f;
				prev_dist = 0;
			}
			else if (0 < i < controlPoints.size() - 1) {
				arc_length = prev_dist + glm::length(controlPoints[i].transform.translation - controlPoints[i - 1].transform.translation);
				prev_dist = arc_length;
			}
			else {
				arc_length = 1.0f;
			}
			
			forwardDiffTable.push_back(std::make_pair(parametric_value, arc_length));

		}

		// Normalize arc lengths
		normalizeForwardDifferencingTable();
	}

	void Animator::normalizeForwardDifferencingTable()
	{
		float total_arc_length = forwardDiffTable[controlPoints.size() - 1].second;
		for (int i = 0; i < forwardDiffTable.size(); i++)
		{
			forwardDiffTable[i].second = forwardDiffTable[i].second / total_arc_length;
		}
	}

	void Animator::printForwardDifferencingTable()
	{
		int index = 0;
		std::cout << "================ FORWARD DIFFERENCE TABLE ==================" << std::endl;
		for (auto entry : forwardDiffTable)
		{
			std::cout << "index: " << index << "   Param: " << entry.first << "      Arc_length: " << entry.second << std::endl;
			index++;
		}
		std::cout << "=============================================================" << std::endl;
	}


}