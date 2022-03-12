#include "l_system.hpp"
#include <iostream>


namespace vmc {
	LSystem::LSystem(std::vector<std::string> prodRules, std::string axiom, glm::vec3 rootPos, int n, float delta): axiomState{axiom}, rootPosition{rootPos}, maxIterations{n}, delta{delta}
	{
		for (std::string const& r : prodRules) {
			productionRules.push_back(ProductionRule{ r, 1.0 });
		}
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
	}

	void LSystem::printAxiomState()
	{
		std::cout << "L-System state: " << axiomState << std::endl;
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