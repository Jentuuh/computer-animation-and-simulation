#pragma once
#include <memory>

namespace vae {
	class Joint;
	class Link
	{
	public:
		Link(float a, float alpha);

		float get_a() { return a; };
		float get_alpha() { return alpha; };
		std::shared_ptr<Joint> getNextJoint() { return nextJoint; };
		void setNextJoint(std::shared_ptr<Joint> next);

	private:
		// Denavit-Hartenberg parameters (2/4)
		float a;
		float alpha;

		std::shared_ptr<Joint> nextJoint;
	};
}

