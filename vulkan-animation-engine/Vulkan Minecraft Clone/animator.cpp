#include "animator.hpp"

#include <algorithm>
#include <iostream>

namespace vae {

	//void Animator::advanceTime(float deltaTime)
	//{
	//	timePassed += deltaTime;
	//}

	void Animator::updateAnimatable()
	{
		updateAnimatedObjects();
	}


	bool Animator::containsObject(VmcGameObject* gameObj)
	{
		for (auto obj : animatedObjects)
		{
			if (obj == gameObj)
				return true;
		}
		return false;
	}


	void Animator::addAnimatedObject(VmcGameObject* gameObject)
	{
		gameObject->onPathAnimator = true;
		animatedObjects.push_back(gameObject);
	}

	void Animator::removeAnimatedObject()
	{
		for (auto& o : animatedObjects)
		{
			o->onPathAnimator = false;
		}
		animatedObjects.clear();
	}

	void Animator::updateAnimatedObjects()
	{
		glm::vec3 newPos = calculateNextPositionSpeedControlled();
		glm::vec3 newRot = calculateIntermediateRotation();
		for (auto gameObj : animatedObjects)
		{
			gameObj->setPosition(newPos);
			gameObj->setRotation(newRot);
		}
	}

	void Animator::buildForwardDifferencingTable()
	{
		forwardDiffTable.clear();
		std::vector<TransformComponent>& curvePoints = getCurvePoints();
		// Calculate parametric values for each point as well as the arc lengths, incrementally
		for (int i = 0; i < curvePoints.size(); i++)
		{
			float parametric_value = (float)i / (curvePoints.size() - 1);
			float arc_length = 0.0f;
			float prev_dist;

			if (i == 0)
			{
				arc_length = 0.0f;
				prev_dist = 0;
			}
			else if (0 < i < curvePoints.size() - 1) {
				arc_length = prev_dist + glm::length(curvePoints[i].translation - curvePoints[i - 1].translation);
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
		float total_arc_length = forwardDiffTable[forwardDiffTable.size() - 1].second;
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

	// Calculates the traversed distance (arc length fraction) based on the time that has passed 
	float Animator::distanceTimeFuncSine()
	{
		// 1/2 sin(3x + (pi/2)) + 1/2
		float distanceFraction = 1 - (0.5f * glm::sin(3 * (timePassed / duration) + (glm::pi<float>() / 2)) + 0.5f);
		// Reset if we've reached the end of the animation loop
		if (timePassed > duration) {
			timePassed = 0.0f;
		}
		return distanceFraction;
	}

	float Animator::distanceTimeFuncLinear()
	{
		// Linear relation between distance and time passed
		float distanceFraction = timePassed / duration;

		// Reset if we've reached the end of the animation loop
		if (timePassed > duration) {
			timePassed = 0.0f;
		}
		return distanceFraction;
	}

	float Animator::distanceTimeFuncParabolic()
	{
		// y = x^2
		float distanceFraction = pow(timePassed / duration, 2);
		return distanceFraction;
	}


	// Finds index in forward differencing table, given a normalized arc length
	int Animator::findUpperIndexOfArcLength(float arcLength)
	{
		// Difference between each value in the forward difference table
		float delta = 1.0f / (float)forwardDiffTable.size();
		// i = (v/d) + 0.5
		int index = (int)((arcLength / delta) + 0.5);

		// Make sure we don't go out of bounds
		return std::clamp<int>(index, 0, forwardDiffTable.size() - 1);;
	}

	int Animator::findLowerIndexOfArcLength(float arcLength)
	{
		// Difference between each value in the forward difference table
		float delta = 1.0f / (float)forwardDiffTable.size();
		// i = (v/d)
		int index = (int)((arcLength / delta));
		return index;
	}
}
