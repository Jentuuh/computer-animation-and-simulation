#include "skeleton.hpp"
#include "simple_render_system.hpp"
#include <iostream>

namespace vae {

	Skeleton::Skeleton(std::shared_ptr<VmcModel> linkModel, std::shared_ptr<VmcModel> jointModel) {
		initSampleSkeleton("../Misc/skeleton.txt", linkModel, jointModel);
		printSkeletonTree(root, 0);
		calculateTransformationMatrices(root);
	}

	void Skeleton::initSampleSkeleton(std::string filePath, std::shared_ptr<VmcModel> linkModel, std::shared_ptr<VmcModel> jointModel)
	{
		std::ifstream infile(filePath);

		std::string line;
		std::vector<std::string> tokens;
		std::shared_ptr<Joint> curr_joint = nullptr;
		std::shared_ptr<Link> curr_link = nullptr;

		int lineNr = 0;
		while (std::getline(infile, line))
		{
			tokens.clear();
			std::string delimiter = " ";

			size_t pos = 0;
			std::string token;
			while ((pos = line.find(delimiter)) != std::string::npos) {
				token = line.substr(0, pos);
				tokens.push_back(token);
				line.erase(0, pos + delimiter.length());
			}
			tokens.push_back(line);

			if (tokens[0] == "LINK")
			{
				float a = std::stof(tokens[1]);
				float alpha = std::stof(tokens[2]);

				// Case 1: curr_joint still has children that need to be added
				if (!curr_joint->isComplete())
				{
					std::shared_ptr<Link> new_link = std::make_shared<Link>(a, alpha, linkModel);
					curr_joint->addChild(new_link);
					new_link->setPrevJoint(curr_joint);
					curr_link = new_link;
				}
				// Case 2: curr_joint is already complete
				else {
					// Backtrack up the tree until we find a joint that is not yet complete
					curr_joint = curr_link->getPrevJoint();
					while (!curr_joint->isComplete())
					{
						std::cout << curr_joint->getChildren().size() << std::endl;

						curr_joint = curr_joint->getParent()->getPrevJoint();
					}

					// Add the new link as a child to the uncomplete joint
					std::shared_ptr<Link> new_link = std::make_shared<Link>(a, alpha, linkModel);
					curr_joint->addChild(new_link);
					new_link->setPrevJoint(curr_joint);
					curr_link = new_link;
				}
			}
			else if (tokens[0] == "JOINT")
			{
				float theta = std::stof(tokens[1]);
				float theta_min = std::stof(tokens[2]);
				float theta_max = std::stof(tokens[3]);
				float d = std::stof(tokens[4]);
				int num_children = std::stoi(tokens[5]);

				if (lineNr == 0)
				{
					root = std::make_shared<Joint>(nullptr, d, theta, theta_min, theta_max, num_children, jointModel);
					curr_joint = root;
				}
				else {
					std::shared_ptr<Joint> new_joint = std::make_shared<Joint>(curr_link, d, theta, theta_min, theta_max, num_children, jointModel);
					curr_link->setNextJoint(new_joint);
					curr_joint = new_joint;
				}
			}

			lineNr++;
		}
	}

	void Skeleton::printSkeletonTree(std::shared_ptr<Joint> rootNode, int treeLevel)
	{
		std::cout << "JOINT on level " << treeLevel;
		std::cout << ", Children links: " << rootNode->getChildren().size() << std::endl;

		for (auto c : rootNode->getChildren())
		{
			if (c->getNextJoint() != nullptr) {
				printSkeletonTree(c->getNextJoint(), treeLevel + 1);
			}
		}

		if (rootNode == root)
			std::cout << "===========================================================" << std::endl;
	}

	void Skeleton::resetTreeTraversal(std::shared_ptr<Joint> rootNode)
	{
		rootNode->resetAmountChildrenTraversed();
		for (auto c : rootNode->getChildren())
		{
			if (c->getNextJoint() == nullptr)
				continue;
			resetTreeTraversal(c->getNextJoint());
		}
	}

	void Skeleton::calculateTransformationMatrices(std::shared_ptr<Joint> rootNode)
	{
		rootNode->calculateLocalTransform();
		for (auto c : rootNode->getChildren())
		{
			if(c->getNextJoint() != nullptr)
				calculateTransformationMatrices(c->getNextJoint());
		}
		if (rootNode == root)
			root->updateGlobalTransform(root->transform.globalTransform);

	}

	void Skeleton::drawSkeleton(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<Joint> root, glm::highp_mat4& projectionView)
	{
		// Draw joint (root)
		TestPushConstant pushSkeleton{};

		auto modelMatrix = root->transform.globalTransform;
		glm::vec4 pos = projectionView * modelMatrix * glm::vec4{ .0f, .0f, .0f, 1.0f };

		// std::cout << glm::to_string(pos) << std::endl;

		pushSkeleton.transform = projectionView * modelMatrix;
		pushSkeleton.normalMatrix = glm::mat3(1.0f);
		pushSkeleton.color = { 1.0f, 1.0f, .5f };

		vkCmdPushConstants(commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(TestPushConstant),
			&pushSkeleton);

		root->jointModel->bind(commandBuffer);
		root->jointModel->draw(commandBuffer);

		// Draw children (links) and further joints on that link (they are in the same frame as the joint so they use the same model matrix)
		for (auto c : root->getChildren())
		{
			//c->linkModel->bind(commandBuffer);
			//c->linkModel->draw(commandBuffer);

			if (c->getNextJoint() != nullptr)
				drawSkeleton(commandBuffer, pipelineLayout, c->getNextJoint(), projectionView);
		}
	}
}