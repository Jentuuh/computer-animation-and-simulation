#include "skeleton2.hpp"
#include <iostream>

namespace vae {
	Skeleton2::Skeleton2(std::shared_ptr<VmcModel> boneMod) :boneModel{ boneMod }, Animatable(0.0f, 4.0f) {}

	void Skeleton2::updateAnimatable()
	{
		int amountKeyFrames = root->getKeyFrames().size();
		if (amountKeyFrames == 0)
			return;
		float timePassedFraction = timePassed / duration;
		float kfFraction = 1.0 / (amountKeyFrames - 1);
		float kfIndex = std::floorf(timePassedFraction / kfFraction);
		float currentKfFraction = (timePassedFraction / kfFraction) - kfIndex;

		for(auto b: boneData)
		{
			b->updateAnimatable(kfIndex, currentKfFraction);
		}
	}

	void Skeleton2::cleanUpAnimatable()
	{
		// Skeleton has no cleanup after animation
	}

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


	void Skeleton2::addKeyFrame()
	{
		Bone* curr = root;
		while (curr != nullptr)
		{
			curr->addKeyFrame();
			curr = curr->getChild();
		}
	}
	
}