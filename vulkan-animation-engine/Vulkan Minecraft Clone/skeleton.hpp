#pragma once
#include "joint.hpp"

#include <glm/gtx/string_cast.hpp>

#include <sstream>
#include <fstream>
#include <memory>
#include <string>


namespace vae {
	class Skeleton
	{
	public:
		Skeleton(std::shared_ptr<VmcModel> linkModel, std::shared_ptr<VmcModel> jointModel);
		void initSampleSkeleton(std::string filePath, std::shared_ptr<VmcModel> linkModel, std::shared_ptr<VmcModel> jointModel);
		void printSkeletonTree(std::shared_ptr<Joint> rootNode, int treeLevel);
		void resetTreeTraversal(std::shared_ptr<Joint> rootNode);
		void calculateTransformationMatrices(std::shared_ptr<Joint> rootNode);
		void drawSkeleton(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<Joint> rootNode, glm::highp_mat4& projectionView);

		std::shared_ptr<Joint> getRoot() { return root; };
		
	private:
		std::shared_ptr<Joint> root;
	};
}

