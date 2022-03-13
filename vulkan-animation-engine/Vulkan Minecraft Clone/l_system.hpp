#pragma once
#include "prod_rule.hpp"
#include "vmc_game_object.hpp"

// std 
#include <vector>
#include <stack>
#include <string>

// glm
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace vmc {
	struct TurtleState {
		glm::vec3 pos;
		glm::vec3 dir;
	};

	const float SEGMENT_LENGTH = .05f;

	class LSystem
	{
	public:
		LSystem(std::vector<std::string> prodRules, std::string axiom, glm::vec3 rootPos, int n, float delta);
		std::vector<TransformComponent>& getRenderPoints() { return renderPoints; };
		void iterate();
		void render();
		void printAxiomState();

	private:
		void drawSegment(glm::vec3 posStart, glm::vec3 posEnd, glm::vec3 scale);
		std::vector<ProductionRule> getMatchingRules(std::vector<char>& leftSyms);

		std::string axiomState;

		std::stack<TurtleState> turtleStack;
		std::vector<ProductionRule> productionRules;
		std::vector<TransformComponent> renderPoints;

		glm::vec3 rootPosition;
		int currentIteration = 1;
		int maxIterations;	// Iterations to perform 
		float delta;		// Turning angle
	};
}
