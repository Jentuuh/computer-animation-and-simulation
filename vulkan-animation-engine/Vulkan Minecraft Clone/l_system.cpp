#include "l_system.hpp"
#include <iostream>


namespace vmc {
	LSystem::LSystem(std::vector<std::string> prodRules, std::string axiom, glm::vec3 rootPos, int n, float delta): axiomState{axiom}, rootPosition{rootPos}, maxIterations{n}, delta{delta}
	{
		// Initialize production rules
		for (std::string const& r : prodRules) {
			productionRules.push_back(ProductionRule{ r, 1.0 });
		}

		// Initialize turtle stack (pos: origin, direction: upwards)
		turtleStack.push({ {.0f, .0f, .0f}, {.0f, -1.0f, .0f} });
		render();
	}

	void LSystem::iterate()
	{
		if (currentIteration >= maxIterations)
			return;

		std::string newState = "";
		for (char c : axiomState) 
		{
			std::vector<char> leftsymbols{};
			leftsymbols.push_back(c);

			std::vector<ProductionRule> matchingRules = getMatchingRules(leftsymbols);

			// If no rule matches, just keep the symbol
			if (matchingRules.size() == 0)
			{
				newState.push_back(c);
			}
			// Else, replace the symbol with the right-hand side of the matching rule
			else {
				// TODO: probabilistic/random selection of rule if there are multiple matching rules
				std::string replacementString = matchingRules[0].getReplacementString();
				newState.append(replacementString);
			}
		}
		currentIteration++;
		axiomState = newState;
		render();
	}

	void LSystem::render()
	{
		for (auto c : axiomState)
		{
			TurtleState currState = turtleStack.top();

			switch (c)
			{
			case 'F':
				drawSegment(currState.pos, currState.pos + SEGMENT_LENGTH * currState.dir, { .5f, .5f, .5f });
				turtleStack.top().pos = currState.pos + SEGMENT_LENGTH * currState.dir;
				break;
			case '+':
				currState.dir = glm::rotateZ(currState.dir, glm::radians(delta));
				turtleStack.pop();
				turtleStack.push(currState);
				break;
			case '-':
				currState.dir = glm::rotateZ(currState.dir, -glm::radians(delta));
				turtleStack.pop();
				turtleStack.push(currState);
				break;
			case '[':
				turtleStack.push(currState);
				//turtleStack.push({ currState.pos, {.0f, -1.0f, .0f} });
				break;
			case ']':
				turtleStack.pop();
				break;
			default:
				break;
			}

		}
	}

	void LSystem::printAxiomState()
	{
		std::cout << "L-System state: " << axiomState << std::endl;
	}

	void LSystem::drawSegment(glm::vec3 posStart, glm::vec3 posEnd, glm::vec3 scale)
	{
		glm::vec3 direction = posEnd - posStart;

		for (float i = .0f; i <= 1.0f; i += 0.05f)
		{
			glm::vec3 pointPos = posStart + i * direction;
			TransformComponent transform{};
			transform.translation = pointPos;
			transform.rotation = { .0f, .0f, .0f };
			transform.scale = { .005f, .005f, .005f };;
			renderPoints.push_back(transform);
		}
	}

	std::vector<ProductionRule> LSystem::getMatchingRules(std::vector<char>& leftSyms)
	{
		std::vector<ProductionRule> matchingRules{};

		for (auto & r : productionRules)
		{
			if (r.matches(leftSyms)) {
				matchingRules.push_back(r);
			}
		}
		return matchingRules;
	}

}