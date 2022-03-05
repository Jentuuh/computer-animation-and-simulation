#pragma once
#include "vmc_game_object.hpp"

// std
#include <utility>


namespace vmc {
	class Animator
	{
	public:
		Animator();

		void addControlPoint(glm::vec3 pos, glm::vec3 color, std::shared_ptr<VmcModel> model);
		std::vector<VmcGameObject>& getControlPoints() { return controlPoints; };

		void advanceTime(float deltaTime);
		glm::vec3 calculateNextPositionLinearInterp(float deltaTime);
		glm::vec3 calculateNextPositionSpeedControlled();
		glm::vec3 calculateNextRotationParabolic();

		void buildForwardDifferencingTable();
		void printForwardDifferencingTable();

		VmcGameObject& getCurrentControlPoint() { return controlPoints[current_cp]; };

	private:
		void advanceToNextControlPoint();
		void normalizeForwardDifferencingTable();

		float distanceTimeFuncSine();
		float distanceTimeFuncLinear();
		float distanceTimeFuncParabolic();

		int findUpperIndexOfArcLength(float arcLength);
		int findLowerIndexOfArcLength(float arcLength);

		std::vector<VmcGameObject> controlPoints;
		std::vector<std::pair<float, float>> forwardDiffTable;

		int current_cp;
		float movementSpeed;
		float pathProgress;		// Fraction of path that has already been traversed
		float timePassed;		// Time that has already passed
		float totalTime;		// Total time duration of the animation
	};

}
