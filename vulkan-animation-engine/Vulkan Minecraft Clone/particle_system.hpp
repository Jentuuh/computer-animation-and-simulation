#pragma once
#include "rigid_body.hpp"

#include <glm/glm.hpp>

namespace vae {
	struct ParticleArea {
		float minX;
		float maxX;
		float minY;
		float maxY;
		float minZ;
		float maxZ;
	};

	class ParticleSystem
	{
	public:
		ParticleSystem(glm::vec3 pos, std::shared_ptr<VmcModel> particleModel);

		void generateParticles(std::vector<RigidBody>& particleStorage);

		glm::vec3 position;
		glm::vec3 shootDirection;

		float power;
		float angleDeviation;

		bool isOn = true;

	private:
		std::shared_ptr<VmcModel> particleModel;
	};
}
