#pragma once
#include "vmc_game_object.hpp"
#include "spline.hpp"

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// std
#include <vector>

// Abstract Animator class
namespace vae {
	class Animator 
	{
	public:
		Animator(glm::vec3 pos, glm::vec3 startOrientation, glm::vec3 endOrientation, float animationTime) : position{ pos }, 
			totalTime{ animationTime }, startOrientation{ startOrientation }, 
			endOrientation{endOrientation} {};

		void advanceTime(float deltaTime);
		virtual glm::vec3 calculateNextPositionSpeedControlled() = 0;
		virtual glm::vec3 calculateIntermediateRotation() = 0;

		virtual glm::vec3 calculateNextRotationParabolic() = 0;
		virtual std::vector<TransformComponent>& getCurvePoints() = 0;
		virtual std::vector<VmcGameObject>& getControlPoints() = 0;
		virtual void updateControlAndCurvePoints() = 0;
		virtual void moveCurrentControlPoint(MoveDirection d, float dt) = 0;
		virtual void selectNextControlPoint() = 0;


		std::vector<std::pair<float, float>> getForwardDiffTable() { return forwardDiffTable; };
		float getTimePassed() { return timePassed; };
		float& getTotalTime() { return totalTime; };
		glm::vec3& getPosition() { return position; };
		glm::vec3& getStartOrientation() { return startOrientation; }
		glm::vec3& getEndOrientation() { return endOrientation; }
		bool containsObject(VmcGameObject* gameObj); 

		void addAnimatedObject(VmcGameObject* gameObject);
		void updateAnimatedObjects();
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

		glm::vec3 position;
		glm::vec3 startOrientation{};
		glm::vec3 endOrientation{};

		std::vector<VmcGameObject*> animatedObjects{};

		std::vector<std::pair<float, float>> forwardDiffTable;
		float timePassed;		// Time that has already passed
		float totalTime;		// Total time duration of the animation
	};
}