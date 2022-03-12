#include "spline_animator.hpp"

// std
#include <iostream>

namespace vmc {

	SplineAnimator::SplineAnimator(std::vector<ControlPoint> controlPoints)
	{
		for (ControlPoint& cp : controlPoints)
		{
			splineCurve.addControlPoint(cp.pos, cp.col, cp.model);
		}

		timePassed = 0.0f;
		totalTime = 4.0f;		// 4 seconds ( TODO: put this as a parameter in the constructor)
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

	std::vector<TransformComponent>& SplineAnimator::getCurvePoints()
	{
		return splineCurve.getCurvePoints();
	}

	std::vector<VmcGameObject>& SplineAnimator::getControlPoints()
	{
		return splineCurve.getControlPoints();
	}

	void SplineAnimator::moveCurrentControlPoint(MoveDirection d, float dt)
	{
		splineCurve.moveCurrentControlPoint(d, dt);
	}

	void SplineAnimator::selectNextControlPoint()
	{
		splineCurve.selectNextControlPoint();
	}
}
