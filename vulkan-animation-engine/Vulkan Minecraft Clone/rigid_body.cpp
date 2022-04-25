#include "rigid_body.hpp"


namespace vae {
	glm::mat4 ObjectState::mat4()
	{
		glm::mat4 result = glm::mat4{
			{0.2f * rotMat[0], 0.0f},
			{0.2f * rotMat[1], 0.0f},
			{0.2f * rotMat[2], 0.0f},
			{pos.x, pos.y, pos.z, 1.0f} };

        return result;
	}

	glm::mat4 ObjectState::normalMatrix()
	{
		return (1.f / 0.2f) * rotMat;
	}

	RigidBody::RigidBody(std::vector<std::pair<glm::vec3, float>> massPoints, bool gravity, std::shared_ptr<VmcModel> model): model{model}
	{
		mass = 0.0f;
		glm::vec3 positionSummed = { .0f, .0f, .0f };
		for (auto& p : massPoints)
		{
			positionSummed += p.second * p.first;
			mass += p.second;
			massPts.push_back(p);
		}

		S.pos = positionSummed / mass;

		// Inertia tensor object
		float I_xx = .0f;
		float I_yy = .0f;
		float I_zz = .0f;

		float I_xy = .0f;
		float I_xz = .0f;
		float I_yz = .0f;
		for (auto& p : massPts)
		{
			I_xx += p.second * (powf(p.first.y, 2) + powf(p.first.z, 2));
			I_yy += p.second * (powf(p.first.x, 2) + powf(p.first.z, 2));
			I_zz += p.second * (powf(p.first.x, 2) + powf(p.first.y, 2));

			I_xy += p.second * p.first.x * p.first.y;
			I_xz += p.second * p.first.x * p.first.z;
			I_yz += p.second * p.first.y * p.first.z;
		}
		inertiaObject = { {I_xx, I_xy, I_xz},{I_xy, I_yy, I_yz},{I_xz, I_yz, I_zz} };

		S.rotMat = glm::mat3(1.0f);
		inertiaTensor = S.rotMat * inertiaObject * glm::transpose(S.rotMat);

		resultingForce = { .0f, .0f, .0f };
		resultingTorque = { .0f, .0f, .0f };

		S.linearImpulse = { .0f, .0f, .0f };
		S.angularImpulse = { .0f, .0f, .0f };

		if (gravity)
			applyForce({ .0f, 9.81f, .0f });

		currRotationAngle = 0.0f;
	}

	void RigidBody::applyForce(glm::vec3 forceVector)
	{
		resultingForce += forceVector;
	}

	void RigidBody::applyTorque(glm::vec3 torqueVector)
	{
		resultingTorque += torqueVector;
	}

	void RigidBody::updateState(float dt)
	{
		// LINEAR MOVEMENT
		// x(t_i) = x(t_i-1) + v*dt 
		S.pos = S.pos + dt * getTranslationalSpeed();
		// v(t_i) = v(t_i-1) + a*dt 
		glm::vec3 newLinearSpeed = getTranslationalSpeed() + dt * resultingForce / mass;
		S.linearImpulse = newLinearSpeed * mass;

		// ANGULAR MOVEMENT
		// The direction of the torque (or angular speed, since both have the same direction) represents our rotation axis, 
		// we use the current angular speed and time difference to calculate the angle that we need to rotate.
		currRotationAngle += dt * glm::length(getAngularSpeed());
		currRotationAngle = fmod(currRotationAngle, glm::pi<float>() * 2);

		S.rotMat = glm::rotate(glm::mat4(1.0f), currRotationAngle, glm::normalize(resultingTorque));

		// omega(t_i) = omega(t_i - 1) + alpha*dt
		glm::vec3 newRotSpeed = getAngularSpeed() + dt * getAngularAcceleration();

		// I(t) = R(t) * I_obj * R(t)^T
		inertiaTensor = S.rotMat * inertiaObject * glm::transpose(S.rotMat);
		S.angularImpulse = inertiaTensor * newRotSpeed;
	}
}