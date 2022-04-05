#include "skeleton.hpp"
#include <iostream>

namespace vae {

	Skeleton::Skeleton() {
		root = std::make_shared<Joint>(nullptr, 0.0f, 0.0f, 0.0f, glm::pi<float>() / 2);
		initSampleSkeleton();
		calculateTransformationMatrices();
	}

	void Skeleton::initSampleSkeleton()
	{
		// Shoulder(root) --> upper arm (link) -> elbow (joint) --> lower arm (link) 
		std::shared_ptr<Link> upperarm1 = std::make_shared<Link>(2.0f, 0.0f);
		std::shared_ptr<Joint> elbow1 = std::make_shared<Joint>(upperarm1, 1.0f, 0.0f, 0.0f, glm::pi<float>()/2);
		upperarm1->setNextJoint(elbow1);
		root->addChild(upperarm1);
		std::shared_ptr<Link> lowerarm1 = std::make_shared<Link>(2.0f, 0.0f);
		elbow1->addChild(lowerarm1);
	}

	void Skeleton::calculateTransformationMatrices()
	{
		std::shared_ptr<Joint> curr = root;

		// Update all local transforms
		while (curr != nullptr)
		{
			curr->calculateLocalTransform();

			// TODO: Make sure that you can do this for more than one link per joint (recursive tree traversal?)
			curr = curr->getChildren()[0]->getNextJoint();
		}

		// Update all global transforms
		root->updateGlobalTransform(root->transform.globalTransform);
	}
}