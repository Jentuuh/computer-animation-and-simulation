#pragma once
#include "vmc_game_object.hpp"
#include "spline.hpp"

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// std
#include <vector>

// Abstract Animator class
namespace vmc {
	class Animator 
	{
	public:
		void advanceTime(float deltaTime);
		virtual glm::vec3 calculateNextPositionSpeedControlled() = 0;
		virtual glm::vec3 calculateNextRotationParabolic() = 0;
		virtual std::vector<TransformComponent>& getCurvePoints() = 0;
		virtual std::vector<VmcGameObject>& getControlPoints() = 0;
		virtual void moveCurrentControlPoint(MoveDirection d, float dt) = 0;
		virtual void selectNextControlPoint() = 0;


		std::vector<std::pair<float, float>> getForwardDiffTable() { return forwardDiffTable; };
		float getTimePassed() { return timePassed; };
		float getTotalTime() { return totalTime; };


		void buildForwardDifferencingTable();
		void printForwardDifferencingTable();

	private:
		void normalizeForwardDifferencingTable();

	protected:
		float distanceTimeFuncSine();
		float distanceTimeFuncLinear();
		float distanceTimeFuncParabolic();

		int findUpperIndexOfArcLength(float arcLength);
		int findLowerIndexOfArcLength(float arcLength);

		std::vector<std::pair<float, float>> forwardDiffTable;
		float timePassed;		// Time that has already passed
		float totalTime;		// Total time duration of the animation
	};
}