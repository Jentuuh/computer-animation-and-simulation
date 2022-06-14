#include "skeleton2.hpp"
#include "simple_render_system.hpp"
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

	void Skeleton2::render(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, std::shared_ptr<VmcModel> pointModel)
	{
		Bone * curr = root;
		while (curr != nullptr)
		{
			curr->render(commandBuffer, pipelineLayout, boneModel);
			curr = curr->getChild();
		}

		if (drawIKTarget)
		{
			glm::mat4 targetModelMatrix = glm::translate(glm::mat4(1.0f), focusPoint) * glm::scale(glm::vec3{0.1f, 0.1f, 0.1f});
			TestPushConstant pushTargetPoint{};
			pushTargetPoint.modelMatrix = targetModelMatrix;
			pushTargetPoint.normalMatrix = glm::mat4(1.0f);
			pushTargetPoint.color = { 1.0f, 0.0f, 0.0f };

			vkCmdPushConstants(commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TestPushConstant),
				&pushTargetPoint);

			pointModel->bind(commandBuffer);
			pointModel->draw(commandBuffer);
		}
	}

	std::vector<glm::vec3> Skeleton2::FK()
	{
		std::vector<glm::vec3> points;
		for (auto b : boneData)
		{
			glm::mat4 globalTransformationMatrix = b->getTransform();
			points.push_back(glm::vec3(globalTransformationMatrix[3].x, globalTransformationMatrix[3].y, globalTransformationMatrix[3].z));
		}
		return points;
	}


	void Skeleton2::solveIK(int maxIterations, float errorMin)
	{
		for (auto b : boneData)
		{
			float rotZ = b->getRotation().z;
			b->setRotation(glm::vec3{ 0.0f, 0.0f, rotZ });
		}

		bool solved = false;
		float errorEndToTarget = std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < maxIterations; i++)
		{
			for (int j = boneData.size() - 1; j >= 0; j--)
			{
				std::vector<glm::vec3> pointCoords = FK();

				glm::vec3 end = pointCoords.back();
				glm::vec3 endTarget = focusPoint - end;

				float err = std::sqrtf(std::powf(endTarget.x, 2.0f) + std::powf(endTarget.y, 2.0f));

				if (err < errorMin)
				{
					solved = true;
				}
				else {
					// Calculate the difference and mag
					glm::vec3 curr = pointCoords[j];
					glm::vec3 currEnd = end - curr;
					float currEndMagnitude = std::sqrtf(std::powf(currEnd.x, 2.0f) + std::powf(currEnd.y, 2.0f));

					glm::vec3 currTarget = focusPoint - curr;
					float currTargetMagnitude = std::sqrtf(std::powf(currTarget.x, 2.0f) + std::powf(currTarget.y, 2.0f));
					float endTargetMagnitude = currEndMagnitude * currTargetMagnitude;
					
					float cosRotAngle;
					float sinRotAngle;
					if (endTargetMagnitude <= 0.0001)
					{
						cosRotAngle = 1;
						sinRotAngle = 0;
					}
					else {
						cosRotAngle = (currEnd.x * currTarget.x + currEnd.y * currTarget.y) / endTargetMagnitude;
						sinRotAngle = (currEnd.x * currTarget.y - currEnd.y * currTarget.x) / endTargetMagnitude;
					}
					float rotAngle = std::acosf(std::max<float>(-1.0f, std::min<float>(1.0f, cosRotAngle)));
					
					if (sinRotAngle < 0.0f) {
						rotAngle = -rotAngle;
					}

					// Update current joint + angle values
					boneData[j]->setRotation(boneData[j]->getRotation() + glm::vec3{ 0.0f, 0.0f, std::fmod((rotAngle * 180 / glm::pi<float>()), 360.0f) });
				
				}
				if (solved)
					break;
			}
		}
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