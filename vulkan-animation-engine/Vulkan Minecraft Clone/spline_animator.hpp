#pragma once
#include "spline.hpp"

namespace vmc {
	class SplineAnimator
	{
	public:
		SplineAnimator();

		Spline& getSpline() { return splineCurve; };

		void advanceTime(float deltaTime);
		glm::vec3 calculateNextPositionSpeedControlled();
		glm::vec3 calculateNextRotationParabolic();

		void buildForwardDifferencingTable();
		void printForwardDifferencingTable();


	private:
		void normalizeForwardDifferencingTable();

		float distanceTimeFuncSine();
		float distanceTimeFuncLinear();
		float distanceTimeFuncParabolic();

		int findUpperIndexOfArcLength(float arcLength);
		int findLowerIndexOfArcLength(float arcLength);

		Spline splineCurve;
		std::vector<std::pair<float, float>> forwardDiffTable;

		float timePassed;		// Time that has already passed
		float totalTime;		// Total time duration of the animation
	};
}
