#include "spline_animator.hpp"

// std
#include <iostream>

namespace vae {

	SplineAnimator::SplineAnimator(glm::vec3 pos, glm::vec3 startOrientation,
									glm::vec3 endOrientation, std::vector<ControlPoint> controlPoints, 
									float animationTime) : Animator(pos, startOrientation, endOrientation, animationTime)
	{
		for (ControlPoint& cp : controlPoints)
		{
			splineCurve.addControlPoint(cp.pos, cp.col, cp.model, position);
		}
		timePassed = 0.0f;
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

	glm::vec3 SplineAnimator::calculateIntermediateRotation()
	{
		glm::vec3 diff = endOrientation - startOrientation;

		float timePassedNormalized = distanceTimeFuncParabolic();
		return startOrientation + timePassedNormalized * diff;
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

	void SplineAnimator::addControlPoint(ControlPoint newControlPoint, glm::vec3 offset)
	{
		splineCurve.addControlPoint(position + newControlPoint.pos, newControlPoint.col, newControlPoint.model, offset);
		splineCurve.generateSplineSegments();
		buildForwardDifferencingTable();
	}

	void SplineAnimator::removeControlPoint(int index)
	{
		splineCurve.getControlPoints().erase(splineCurve.getControlPoints().begin() + index);
		splineCurve.generateSplineSegments();
		buildForwardDifferencingTable();
	}


	void SplineAnimator::updateControlAndCurvePoints()
	{
		splineCurve.updateControlPointsAndCurvePointsPositions(position);
	};

	void SplineAnimator::moveCurrentControlPoint(MoveDirection d, float dt)
	{
		splineCurve.moveCurrentControlPoint(d, dt);
	}

	void SplineAnimator::selectNextControlPoint()
	{
		splineCurve.selectNextControlPoint();
	}
}
