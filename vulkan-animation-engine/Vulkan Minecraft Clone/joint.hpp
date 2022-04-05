#pragma once
#include "link.hpp"
#include "vmc_model.hpp"

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vae {

	struct JointTransform {
		glm::mat4 localTransform;	// Applies to current joint + children
		glm::mat4 globalTransform;	// Result of transforming parents of this joint
	};

	class Joint
	{
	public:
		Joint(std::shared_ptr<Link> parent, float d, float theta, float theta_min, float theta_max);

		std::vector<std::shared_ptr<Link>>& getChildren() { return children; };

		void addChild(std::shared_ptr<Link> newChild);
		void calculateLocalTransform();
		void updateGlobalTransform(glm::mat4 parentTransform);

		JointTransform transform;
	private:
		// Denavit-Hartenberg parameters (2/4)
		float d;
		float theta;
		float theta_min;
		float theta_max;

		std::shared_ptr<Link> parent;
		std::vector<std::shared_ptr<Link>> children;
	};

}
