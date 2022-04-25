#pragma once
#include "vmc_model.hpp"

// glm
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

// std
#include <vector>
#include <iostream>


namespace vae {

	struct ObjectState {
		glm::vec3 pos;
		glm::mat3 rotMat;
		glm::vec3 linearImpulse;
		glm::vec3 angularImpulse;

		glm::mat4 mat4();
		glm::mat4 normalMatrix();
	};

	class RigidBody
	{
	public:
		RigidBody(std::vector<std::pair<glm::vec3, float>> massPoints, bool gravity, std::shared_ptr<VmcModel> model);

		void applyForce(glm::vec3 forceVector);
		void applyTorque(glm::vec3 torqueVector);
		void updateState(float dt);

		glm::vec3 getTranslationalSpeed() { return S.linearImpulse / mass; };
		glm::vec3 getAngularSpeed() { return glm::inverse(inertiaTensor) * S.angularImpulse; };
		glm::vec3 getAngularAcceleration() { return glm::inverse(inertiaTensor) * resultingTorque; };

		ObjectState S;
		std::shared_ptr<VmcModel> model;
	private:
		glm::vec3 massCenter;
		float mass;
		glm::mat3 inertiaObject;
		glm::mat3 inertiaTensor;

		glm::vec3 resultingForce;
		glm::vec3 resultingTorque;
		std::vector<std::pair<glm::vec3, float>> massPts;
		float currRotationAngle;
	};
}

