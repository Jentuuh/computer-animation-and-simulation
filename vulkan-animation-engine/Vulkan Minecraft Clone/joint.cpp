#include "joint.hpp"
#include <iostream>


namespace vae {

	Joint::Joint(std::shared_ptr<Link> parent, float d, float theta, float theta_min, float theta_max, int amount_children, std::shared_ptr<VmcModel> model) : parent{ parent }, d{ d }, theta{ theta },
		theta_min{ theta_min }, theta_max{ theta_max }, amount_children{ amount_children }, jointModel{ model } {}

	void Joint::addChild(std::shared_ptr<Link> newChild)
	{
		children.push_back(newChild);
		amount_children_traversed++;
	}


	void Joint::calculateLocalTransform()
	{
		if (parent == nullptr){
			// Rotation {0,0,0}
			const float c3 = glm::cos(0);
			const float s3 = glm::sin(0);
			const float c2 = glm::cos(0);
			const float s2 = glm::sin(0);
			const float c1 = glm::cos(0);
			const float s1 = glm::sin(0);

			glm::vec3 scale{ 0.1f, 0.1f, 0.1f };

			transform.globalTransform = glm::mat4(1.0f);
			/*transform.globalTransform = glm::mat4{
				{
					scale.x * (c1 * c3 + s1 * s2 * s3),
					scale.x * (c2 * s3),
					scale.x * (c1 * s2 * s3 - c3 * s1),
					0.0f,
				},
				{
					scale.y * (c3 * s1 * s2 - c1 * s3),
					scale.y * (c2 * c3),
					scale.y * (c1 * c3 * s2 + s1 * s3),
					0.0f,
				},
				{
					scale.z * (c2 * s1),
					scale.z * (-s2),
					scale.z * (c1 * c2),
					0.0f,
				},
				{.0f, .0f, .0f, 1.0f} };*/
		}
		else {
			transform.localTransform = glm::mat4{
				{
					glm::cos(theta), 
					-glm::sin(theta), 
					0, 
					parent->get_a()
				},
				{
					glm::cos(parent->get_alpha()) * glm::sin(theta), 
					glm::cos(parent->get_alpha()) * glm::cos(theta), 
					-glm::sin(parent->get_alpha()), 
					-d * glm::sin(parent->get_alpha())
				},
				{
					glm::sin(parent->get_alpha()) * glm::sin(theta), 
					glm::sin(parent->get_alpha()) * glm::cos(theta), 
					glm::cos(parent->get_alpha()), 
					d * glm::cos(parent->get_alpha())
				},
				{.0f, .0f, .0f, 1.0f}
			};
		}
	}

	void Joint::updateGlobalTransform(glm::mat4 parentTransform)
	{
		if (parent == nullptr) {
			// Rotation {0,0,0}
			const float c3 = glm::cos(0);
			const float s3 = glm::sin(0);
			const float c2 = glm::cos(0);
			const float s2 = glm::sin(0);
			const float c1 = glm::cos(0);
			const float s1 = glm::sin(0);

			glm::vec3 scale{ 0.1f, 0.1f, 0.1f };

			transform.globalTransform = glm::mat4(1.0f);/*glm::mat4{
				{
					scale.x * (c1 * c3 + s1 * s2 * s3),
					scale.x * (c2 * s3),
					scale.x * (c1 * s2 * s3 - c3 * s1),
					0.0f,
				},
				{
					scale.y * (c3 * s1 * s2 - c1 * s3),
					scale.y * (c2 * c3),
					scale.y * (c1 * c3 * s2 + s1 * s3),
					0.0f,
				},
				{
					scale.z * (c2 * s1),
					scale.z * (-s2),
					scale.z * (c1 * c2),
					0.0f,
				},
				{0.0f, 0.0f, 0.0f, 1.0f} };*/
		}
		else {
			// Transform this joint according to its local coordinate frame, with the origin 
			// of that frame in the point defined by the global transformation of the parent.
			transform.globalTransform = parentTransform * transform.localTransform;
		}

		// Recursively update children's global transform matrices as well
		for (std::shared_ptr<Link> c : children)
		{
			if (c->getNextJoint() != nullptr)
			{
				// Next joint needs to be transformed according to the transformation in local coordinate frame space, 
				// with it's origin in the point defined by the global transformation of this joint.
				c->getNextJoint()->updateGlobalTransform(transform.globalTransform);
			}
		}
	}

	bool Joint::isComplete() 
	{ 
		bool completed = amount_children_traversed == amount_children;

		// Reset for future traversal
		//if (completed)
		//	amount_children_traversed = 0;

		return completed; 
	};

}