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
		Joint(std::shared_ptr<Link> parent, float d, float theta, float theta_min, float theta_max, int amount_children, std::shared_ptr<VmcModel> model);

		std::vector<std::shared_ptr<Link>>& getChildren() { return children; };
		std::shared_ptr<Link> getParent() { return parent; };
		int getAmountChildrenTraversed() { return amount_children_traversed; };
		void incAmountChildrenTraversed() { amount_children_traversed++; };
		void resetAmountChildrenTraversed() { amount_children_traversed = 0; };

		void addChild(std::shared_ptr<Link> newChild);
		bool isComplete();
		void calculateLocalTransform();
		void updateGlobalTransform(glm::mat4 parentTransform);

		JointTransform transform;
		std::shared_ptr<VmcModel> jointModel{};

	private:
		// Denavit-Hartenberg parameters (2/4)
		float d;
		float theta;
		float theta_min;
		float theta_max;

		int amount_children_traversed = 0;	// Necessary to check if all children were added when building up/traversing the tree
		int amount_children;

		std::shared_ptr<Link> parent;
		std::vector<std::shared_ptr<Link>> children;
	};

}
