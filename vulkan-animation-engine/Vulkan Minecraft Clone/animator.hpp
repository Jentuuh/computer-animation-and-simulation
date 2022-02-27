#pragma once
#include "vmc_game_object.hpp"


namespace vmc {
	class Animator
	{
	public:
		Animator();

		void addControlPoint(glm::vec3 pos, std::shared_ptr<VmcModel> model);
		std::vector<VmcGameObject>& getControlPoints() { return controlPoints; };
		glm::vec3 calculateNextPosition(float deltaTime);
		VmcGameObject& getCurrentControlPoint() { return controlPoints[current_cp]; };


	private:
		void advanceToNextControlPoint();


		std::vector<VmcGameObject> controlPoints;
		int current_cp;
		float movementSpeed;
		float pathProgress;		// Fraction of path that has already been traversed
	};

}
