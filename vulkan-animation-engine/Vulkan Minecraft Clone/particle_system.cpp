#include "particle_system.hpp"
#include <iostream>

namespace vae {

	ParticleSystem::ParticleSystem(glm::vec3 pos, std::shared_ptr<VmcModel> particleModel) : Animatable(0.0f, 4.0f), position{ pos }, particleModel{particleModel}
	{
		shootDirection = { 0.0f, -1.0f, 0.0f };
		power = 20.0f;
		angleDeviation = 0.2f;
	}


	void ParticleSystem::updateAnimatable()
	{
		if (keyframes.size() == 0)
		{
			return;
		}
		isOn = true;
		float timePassedFraction = timePassed / duration;
		float kfFraction = 1.0 / (keyframes.size() - 1);
		float kfIndex = std::floorf(timePassedFraction / kfFraction);
		float currentKfFraction = (timePassedFraction / kfFraction) - kfIndex;

		glm::vec3 interpolatedPos = keyframes[(int)kfIndex].pos + currentKfFraction * (keyframes[(int)kfIndex + 1].pos - keyframes[(int)kfIndex].pos);
		glm::vec3 interpolatedShootDir = keyframes[(int)kfIndex].shootDir + currentKfFraction * (keyframes[(int)kfIndex + 1].shootDir - keyframes[(int)kfIndex].shootDir);
		float interPolatedPower = keyframes[(int)kfIndex].power + currentKfFraction * (keyframes[(int)kfIndex + 1].power - keyframes[(int)kfIndex].power);

		position = interpolatedPos;
		shootDirection = interpolatedShootDir;
		power = interPolatedPower;
	}

	void ParticleSystem::cleanUpAnimatable()
	{
		isOn = false;
	}

	void ParticleSystem::addKeyFrame()
	{
		keyframes.push_back({ position, shootDirection, power });
	}

	void ParticleSystem::addKeyFrames(std::vector<ParticleKeyFrame> kfs)
	{
		keyframes = kfs;
	}

	void ParticleSystem::deleteKeyFrame(int index)
	{
		keyframes.erase(keyframes.begin() + index);
	}

	void ParticleSystem::generateParticles(std::vector<RigidBody>& particleStorage)
	{
		if (isOn)
		{
			std::vector<std::pair<glm::vec3, float>> massPoints;
			massPoints.push_back(std::make_pair(position + glm::vec3{ 1.0f, 0.0f, 0.0f }, 1.0f));
			massPoints.push_back(std::make_pair(position + glm::vec3{ 0.0f, 1.0f, 0.0f }, 1.0f));
			massPoints.push_back(std::make_pair(position + glm::vec3{ 0.0f, 0.0f, 1.0f }, 1.0f));

			float scale = ((float)rand() / RAND_MAX) * 0.05f;

			float devX = ((float)rand() / RAND_MAX) * angleDeviation;
			float devY = ((float)rand() / RAND_MAX) * angleDeviation;
			float devZ = ((float)rand() / RAND_MAX) * angleDeviation;

			RigidBody particle{ massPoints, true, particleModel, {scale, scale, scale} };
			particle.S.linearImpulse = (glm::normalize(shootDirection + glm::vec3{devX, devY, devZ}) * power);
			particle.applyTorque({ 1.0f, .0f, 1.0f });

			// Make sure particles die after some time
			if (particleStorage.size() > 1000)
			{
				particleStorage.erase(particleStorage.begin());
			}
			particleStorage.push_back(particle);
		}
		else
		{
			if (particleStorage.size() > 0)
				particleStorage.clear();
		}
	}
}