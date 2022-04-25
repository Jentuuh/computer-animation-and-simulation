#pragma once
#include "vmc_model.hpp"

#include <memory>

namespace vae {
	class Joint;
	class Link
	{
	public:
		Link(float a, float alpha, std::shared_ptr<VmcModel> model);

		float get_a() { return a; };
		float get_alpha() { return alpha; };
		std::shared_ptr<Joint> getNextJoint() { return nextJoint; };
		std::shared_ptr<Joint> getPrevJoint() { return prevJoint; };

		void setNextJoint(std::shared_ptr<Joint> next);
		void setPrevJoint(std::shared_ptr<Joint> prev);

		std::shared_ptr<VmcModel> linkModel{};
	private:
		// Denavit-Hartenberg parameters (2/4)
		float a;
		float alpha;

		std::shared_ptr<Joint> nextJoint;
		std::shared_ptr<Joint> prevJoint;
	};
}

