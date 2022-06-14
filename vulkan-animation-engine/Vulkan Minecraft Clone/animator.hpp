#pragma once
#include "vmc_game_object.hpp"
#include "spline.hpp"
#include "animatable.hpp"

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// std
#include <vector>

// Abstract Animator class
namespace vae {
	enum SPEED_CONTROL_FUNCTION {
		SINE,
		LINEAR,
		PARABOLIC
	};

	class Animator : public Animatable
	{
	public:
		Animator(glm::vec3 pos, glm::vec3 startOrientation, glm::vec3 endOrientation, float animationTime, float startTime) : Animatable(startTime, animationTime), position{ pos }, 
			startOrientation{ startOrientation }, 
			endOrientation{endOrientation} {};

		void updateAnimatable();
		void cleanUpAnimatable();
		virtual glm::vec3 calculateNextPositionSpeedControlled() = 0;
		virtual glm::vec3 calculateIntermediateRotation() = 0;

		virtual std::vector<TransformComponent>& getCurvePoints() = 0;
		virtual std::vector<VmcGameObject>& getControlPoints() = 0;
		virtual void updateControlAndCurvePoints() = 0;
		virtual void moveCurrentControlPoint(MoveDirection d, float dt) = 0;
		virtual void selectNextControlPoint() = 0;


		std::vector<std::pair<float, float>> getForwardDiffTable() { return forwardDiffTable; };
		glm::vec3& getPosition() { return position; };
		glm::vec3& getStartOrientation() { return startOrientation; }
		glm::vec3& getEndOrientation() { return endOrientation; }
		int getAmountAnimatedObjects() { return animatedObjects.size(); };
		std::vector<int> getAnimatedObjectIds();
		bool containsObject(VmcGameObject* gameObj); 

		void addAnimatedObject(VmcGameObject* gameObject);
		void removeAnimatedObject();
		void buildForwardDifferencingTable();
		void printForwardDifferencingTable();

		std::string currentObjSelected = "None";
		int speedControl = SINE;
	private:
		void normalizeForwardDifferencingTable();

	protected:
		float distanceTimeFuncSine();
		float distanceTimeFuncLinear();
		float distanceTimeFuncParabolic();

		int findUpperIndexOfArcLength(float arcLength);
		int findLowerIndexOfArcLength(float arcLength);

		void updateAnimatedObjects();

		glm::vec3 position;
		glm::vec3 startOrientation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 endOrientation{ 0.0f, 0.0f, 0.0f };

		std::vector<VmcGameObject*> animatedObjects{};

		std::vector<std::pair<float, float>> forwardDiffTable;
	};
}