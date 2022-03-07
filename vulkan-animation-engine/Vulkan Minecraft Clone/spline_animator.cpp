#include "spline_animator.hpp"

// std
#include <iostream>

namespace vmc {

	SplineAnimator::SplineAnimator()
	{
		timePassed = 0.0f;
		totalTime = 4.0f;		// 4 seconds ( TODO: put this as a parameter in the constructor)
	}

	void SplineAnimator::advanceTime(float deltaTime)
	{
		timePassed += deltaTime;
	}

	glm::vec3 SplineAnimator::calculateNextPositionSpeedControlled()
	{
		// Sine speed control function
		float dist_time = distanceTimeFuncSine();

		// Linear speed control function
		//float dist_time = distanceTimeFuncLinear(deltaTime);

		int index = findUpperIndexOfArcLength(dist_time);

		return splineCurve.getCurvePoints()[index].translation;
	}

	glm::vec3 SplineAnimator::calculateNextRotationParabolic()
	{
		// Normalized fraction of time that has passed
		float timePassedNormalized = distanceTimeFuncParabolic();

		return glm::vec3{ .0f, .0f, timePassedNormalized * 2 * glm::pi<float>() };
	}

	void SplineAnimator::buildForwardDifferencingTable()
	{
		forwardDiffTable.clear();
		std::vector<TransformComponent>& curvePoints = splineCurve.getCurvePoints();

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
			else if (0 < i < splineCurve.getCurvePoints().size() - 1) {
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

	void SplineAnimator::normalizeForwardDifferencingTable()
	{
		float total_arc_length = forwardDiffTable[splineCurve.getCurvePoints().size() - 1].second;
		std::cout << total_arc_length << std::endl;
		for (int i = 0; i < forwardDiffTable.size(); i++)
		{
			forwardDiffTable[i].second = forwardDiffTable[i].second / total_arc_length;
		}
	}

	void SplineAnimator::printForwardDifferencingTable()
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
	float SplineAnimator::distanceTimeFuncSine()
	{
		// 1/2 sin(3x + (pi/2)) + 1/2
		float distanceFraction = 0.5f * glm::sin(3 * (timePassed / totalTime) + (glm::pi<float>() / 2)) + 0.5f;
		// Reset if we've reached the end of the animation loop
		if (timePassed > totalTime) {
			timePassed = 0.0f;
		}
		return distanceFraction;
	}

	float SplineAnimator::distanceTimeFuncLinear()
	{
		// Linear relation between distance and time passed
		float distanceFraction = timePassed / totalTime;

		// Reset if we've reached the end of the animation loop
		if (timePassed > totalTime) {
			timePassed = 0.0f;
		}
		return distanceFraction;
	}

	float SplineAnimator::distanceTimeFuncParabolic()
	{
		// y = x^2
		float distanceFraction = pow(timePassed / totalTime, 2);
		return distanceFraction;
	}


	// Finds index in forward differencing table, given a normalized arc length
	int SplineAnimator::findUpperIndexOfArcLength(float arcLength)
	{
		// Difference between each value in the forward difference table
		float delta = 1.0f / (float)forwardDiffTable.size();
		// i = (v/d) + 0.5
		int index = (int)((arcLength / delta) + 0.5);
		return index;
	}

	int SplineAnimator::findLowerIndexOfArcLength(float arcLength)
	{
		// Difference between each value in the forward difference table
		float delta = 1.0f / (float)forwardDiffTable.size();
		// i = (v/d)
		int index = (int)((arcLength / delta));
		return index;
	}
}
