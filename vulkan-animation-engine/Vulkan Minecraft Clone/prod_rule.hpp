#pragma once

// std
#include <vector>
#include <string>

namespace vmc {
	class ProductionRule
	{
	public:
		ProductionRule(std::string unparsed, float prob);

		std::string getReplacementString();
		bool matches(std::vector<char>& leftSyms);
		void printInfo();
	private:
		std::vector<char> left;
		std::vector<char> right;
		float probability;
	};
}

