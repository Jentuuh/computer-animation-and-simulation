#include "skeleton2.hpp"
#include <iostream>

namespace vae {
	Skeleton2::Skeleton2(std::shared_ptr<VmcModel> boneMod) :boneModel{ boneMod } {}

	void Skeleton2::addRoot(glm::vec3 pos, float len, glm::vec3 rot)
	{
		Bone rootBone{ pos, len, rot };
		boneData.push_back(std::make_shared<Bone>(rootBone));
		root = boneData[0].get();
	}

	void Skeleton2::addBone(float len, glm::vec3 rot)
	{
		Bone newBone{ boneData[boneData.size() - 1].get(), len, rot};
		boneData.push_back(std::make_shared<Bone>(newBone));
		boneData[boneData.size() - 2]->setChild(boneData[boneData.size() - 1].get());
	}

	void Skeleton2::render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout)
	{
		Bone * curr = root;
		while (curr != nullptr)
		{
			curr->render(commandBuffer, pipelineLayout, boneModel);
			curr = curr->getChild();
		}
	}

	void Skeleton2::update()
	{
		boneData.back()->follow(focusPoint);
	}

}