#include "function_animator.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

// std
#include <algorithm>
#include <math.h>
#include <iostream>

namespace vmc {
	FunctionAnimator::FunctionAnimator()
	{
		current_cp = 0;
		pathProgress = 0.0f;
		timePassed = 0.0f;
		totalTime = 4.0f;		// 4 seconds ( TODO: put this as a parameter in the constructor)
		movementSpeed = 30.0f;
	}

	void FunctionAnimator::addControlPoint(glm::vec3 pos, glm::vec3 color, std::shared_ptr<VmcModel> model)
	{
		auto contr_point = VmcGameObject::createGameObject();
		contr_point.model = model;
		contr_point.transform.translation = pos;
		contr_point.transform.scale = { 0.05f, 0.05f, 0.05f };
		contr_point.color = color;

		controlPoints.push_back(std::move(contr_point));
	}

	void FunctionAnimator::advanceToNextControlPoint()
	{
		current_cp = (current_cp + 1) % controlPoints.size();
	}

	void FunctionAnimator::advanceTime(float deltaTime)
	{
		timePassed += deltaTime;
	}


	glm::vec3 FunctionAnimator::calculateNextPositionLinearInterp(float deltaTime)
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

	glm::vec3 FunctionAnimator::calculateNextPositionSpeedControlled()
	{
		// Sine speed control function
		float dist_time = distanceTimeFuncSine();

		// Linear speed control function
		//float dist_time = distanceTimeFuncLinear(deltaTime);

		int index = findUpperIndexOfArcLength(dist_time);

		return controlPoints[index].transform.translation;
	}

	glm::vec3 FunctionAnimator::calculateNextRotationParabolic()
	{
		// Normalized fraction of time that has passed
		float timePassedNormalized = distanceTimeFuncParabolic();

		return glm::vec3{ .0f, .0f, timePassedNormalized * 2 * glm::pi<float>() };
	}


	void FunctionAnimator::buildForwardDifferencingTable()
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

	void FunctionAnimator::normalizeForwardDifferencingTable()
	{
		float total_arc_length = forwardDiffTable[controlPoints.size() - 1].second;
		for (int i = 0; i < forwardDiffTable.size(); i++)
		{
			forwardDiffTable[i].second = forwardDiffTable[i].second / total_arc_length;
		}
	}

	void FunctionAnimator::printForwardDifferencingTable()
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

	// Calculates the traversed distance (arc length fraction) based on the time that has passed 
	float FunctionAnimator::distanceTimeFuncSine()
	{
		// 1/2 sin(3x + (pi/2)) + 1/2
		float distanceFraction = 0.5f * glm::sin(3 * (timePassed / totalTime) + (glm::pi<float>() / 2)) + 0.5f;
		// Reset if we've reached the end of the animation loop
		if (timePassed > totalTime) {
			timePassed = 0.0f;
		}
		return distanceFraction;
	}

	float FunctionAnimator::distanceTimeFuncLinear()
	{
		// Linear relation between distance and time passed
		float distanceFraction = timePassed / totalTime;

		// Reset if we've reached the end of the animation loop
		if (timePassed > totalTime) {
			timePassed = 0.0f;
		}
		return distanceFraction;
	}

	float FunctionAnimator::distanceTimeFuncParabolic()
	{
		// y = x^2
		float distanceFraction = pow(timePassed / totalTime, 2);
		return distanceFraction;
	}


	// Finds index in forward differencing table, given a normalized arc length
	int FunctionAnimator::findUpperIndexOfArcLength(float arcLength)
	{
		// Difference between each value in the forward difference table
		float delta = 1.0f / (float)forwardDiffTable.size();
		// i = (v/d) + 0.5
		int index = (int)((arcLength / delta) + 0.5);
		return index;
	}

	int FunctionAnimator::findLowerIndexOfArcLength(float arcLength)
	{
		// Difference between each value in the forward difference table
		float delta = 1.0f / (float)forwardDiffTable.size();
		// i = (v/d)
		int index = (int)((arcLength / delta));
		return index;
	}

}