#pragma once
#include "prod_rule.hpp"

// std 
#include <vector>
#include <stack>
#include <string>

// glm
#include <glm/glm.hpp>

namespace vmc {
	struct TurtleState {
		glm::vec3 pos;
		glm::vec3 dir;
	};
	class LSystem
	{
	public:
		LSystem(std::vector<std::string> prodRules, std::string axiom, glm::vec3 rootPos, int n, float delta);
		void iterate();
		void render();
		void printAxiomState();

	private:
		std::vector<ProductionRule> getMatchingRules(std::vector<char>& leftSyms);

		std::string axiomState;

		std::stack<TurtleState> turtleStack;
		std::vector<ProductionRule> productionRules;

		glm::vec3 rootPosition;
		int currentIteration = 1;
		int maxIterations;	// Iterations to perform 
		float delta;		// Turning angle
	};
}
