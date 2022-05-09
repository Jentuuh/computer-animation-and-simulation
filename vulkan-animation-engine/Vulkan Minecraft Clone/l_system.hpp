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

// Probabilistic L-System class. Generates a collection of 3D points representing 
// the L-System iteratively according to a set of probabilistic production rules. 
namespace vae {
	struct TurtleState {
		glm::vec3 pos;
		glm::vec3 dir;
	};

	enum VegetationType {
		BUSH,
		LONG_PLANT,
		TREE
	};

	const float SEGMENT_LENGTH = .05f;

	class LSystem
	{
	public:
		LSystem(std::vector<std::pair<std::string, float>> prodRules, std::string axiom, glm::vec3 rootPos, int n, float delta, VegetationType type);
		LSystem(const char* filePath, VegetationType type);
		std::vector<TransformComponent>& getRenderPoints() { return renderPoints; };
		std::string getVegetationType() { return veg_type; };

		void mature();
		void iterate();
		void render();
		void resetTurtleAndRerender();
		void printAxiomState();

		glm::vec3 rootPosition;
		glm::vec3 renderColor;
	private:
		void drawSegment(glm::vec3 posStart, glm::vec3 posEnd, glm::vec3 scale);
		std::vector<ProductionRule> getMatchingRules(std::vector<char>& leftSyms);

		std::string axiomState;

		std::stack<TurtleState> turtleStack;
		std::vector<ProductionRule> productionRules;
		std::vector<TransformComponent> renderPoints;

		int currentIteration = 1;
		int maxIterations;	// Iterations to perform 
		float delta;		// Turning angle

		std::string veg_type;
	};
}
