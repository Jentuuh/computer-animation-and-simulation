#pragma once
#include "rigid_body.hpp"
#include "animatable.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

namespace vae {
	struct ParticleArea {
		float minX;
		float maxX;
		float minY;
		float maxY;
		float minZ;
		float maxZ;
	};

	struct ParticleKeyFrame {
		glm::vec3 pos;
		glm::vec3 shootDir;
		float power;
	};

	class ParticleSystem: public Animatable
	{
	public:
		ParticleSystem(glm::vec3 pos, std::shared_ptr<VmcModel> particleModel);

		void updateAnimatable();
		void cleanUpAnimatable();
		int getAmountKeyFrames() { return keyframes.size(); };
		std::vector<ParticleKeyFrame>& getKeyFrames() { return keyframes; };
		void addKeyFrame();
		void addKeyFrames(std::vector<ParticleKeyFrame> kfs);
		void deleteKeyFrame(int index);
		void generateParticles(std::vector<RigidBody>& particleStorage);

		glm::vec3 position;
		glm::vec3 shootDirection;

		float power;
		float angleDeviation;

		bool isOn = true;

	private:
		std::vector<ParticleKeyFrame> keyframes;
		std::shared_ptr<VmcModel> particleModel;
	};
}
