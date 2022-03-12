#include "prod_rule.hpp"

#include <iostream>

namespace vmc {
	ProductionRule::ProductionRule(std::string unparsed, float prob) : probability{prob}
	{
		// Split arrow
		std::string delimiter = "=>";
		std::string leftString = unparsed.substr(0, unparsed.find(delimiter));
		std::string rightString = unparsed.erase(0, unparsed.find(delimiter) + delimiter.length());

		for (char const& c : leftString) {
			left.push_back(c);
		}
		for (char const& c : rightString) {
			right.push_back(c);
		}
	}

	std::string ProductionRule::getReplacementString()
	{
		std::string rep = "";
		for (char c : right)
		{
			rep.push_back(c);
		}
		return rep;
	}


	bool ProductionRule::matches(std::vector<char>& leftSyms)
	{
		if (left.size() != leftSyms.size())
			return false;

		for (int i = 0; i < leftSyms.size(); i++)
		{
			if (left[i] != leftSyms[i])
				return false;
		}
		return true;
	}

	void ProductionRule::printInfo(){
		std::cout << "======= PRODUCTION RULE =======" << std::endl;

		std::cout << "Left symbol(s):";
		for (char const& c : left) {
			std::cout << " " << c << " ";
		}
		std::cout << std::endl;

		std::cout << "Right symbol(s):";
		for (char const& c : right) {
			std::cout << " " << c << " ";
		}
		std::cout << std::endl;

		std::cout << "Rule probability: " << probability << std::endl;
	}

}