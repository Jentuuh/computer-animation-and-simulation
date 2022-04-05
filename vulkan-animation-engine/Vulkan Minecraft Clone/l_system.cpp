#include "l_system.hpp"
#include <iostream>


namespace vae {
	LSystem::LSystem(std::vector<std::pair<std::string, float>> prodRules, std::string axiom, glm::vec3 rootPos, int n, float delta): axiomState{axiom}, rootPosition{rootPos}, maxIterations{n}, delta{delta}
	{
		// Initialize production rules
		for (auto const& r : prodRules) {
			productionRules.push_back(ProductionRule{ r.first, r.second });
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
				int random = std::rand() % 100;

				int cumulative = 0;
				for (int i = 0; i < matchingRules.size(); i++)
				{
					// Use cdf to distribute the rules according to the probabilities. E.g. 4 rules each with 0.25 probability, rand. number 
					// between 0-99 is generated and compared to the cdf. As soon as the random number is smaller than the CDF for the current 
					// iteration (x-axis of cdf), we've found the 'winning' rule. 
					// Note that this does not work as expected if there are more than 100 rules, but we assume this will not occur. 
					cumulative += matchingRules[0].getProbablity() * 100;
					if (random <= cumulative)
					{
						std::string replacementString = matchingRules[i].getReplacementString();
						newState.append(replacementString);
						break;
					}
				}
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