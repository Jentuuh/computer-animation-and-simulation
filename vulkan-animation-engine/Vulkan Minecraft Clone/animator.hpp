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
		glm::vec3 calculateNextPosition(float deltaTime);
		void buildForwardDifferencingTable();
		void printForwardDifferencingTable();

		VmcGameObject& getCurrentControlPoint() { return controlPoints[current_cp]; };



	private:
		void advanceToNextControlPoint();
		void normalizeForwardDifferencingTable();

		std::vector<VmcGameObject> controlPoints;
		std::vector<std::pair<float, float>> forwardDiffTable;
		int current_cp;
		float movementSpeed;
		float pathProgress;		// Fraction of path that has already been traversed
	};

}
