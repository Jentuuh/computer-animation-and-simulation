#include "particle_system.hpp"
#include <iostream>

namespace vae {

	ParticleSystem::ParticleSystem(glm::vec3 pos, std::shared_ptr<VmcModel> particleModel) : position{ pos }, particleModel{particleModel}
	{
		shootDirection = { 0.0f, -1.0f, 0.0f };
		power = 20.0f;
		angleDeviation = 0.2f;
	}


	void ParticleSystem::generateParticles(std::vector<RigidBody>& particleStorage)
	{
		if (isOn)
		{
			std::vector<std::pair<glm::vec3, float>> massPoints;
			massPoints.push_back(std::make_pair(position + glm::vec3{ 1.0f, 0.0f, 0.0f }, 1.0f));
			massPoints.push_back(std::make_pair(position + glm::vec3{ 0.0f, 1.0f, 0.0f }, 1.0f));
			massPoints.push_back(std::make_pair(position + glm::vec3{ 0.0f, 0.0f, 1.0f }, 1.0f));

			float scale = ((float)rand() / RAND_MAX) * 0.2f;

			float devX = ((float)rand() / RAND_MAX) * angleDeviation;
			float devY = ((float)rand() / RAND_MAX) * angleDeviation;
			float devZ = ((float)rand() / RAND_MAX) * angleDeviation;

			RigidBody particle{ massPoints, true, particleModel, {scale, scale, scale} };
			particle.S.linearImpulse = (glm::normalize(shootDirection + glm::vec3{devX, devY, devZ})* power);

			// Make sure particles die after some time
			if (particleStorage.size() > 500)
			{
				particleStorage.erase(particleStorage.begin());
			}
			particleStorage.push_back(particle);
		}
	}
}