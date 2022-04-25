#pragma once
#include <glm/glm.hpp>
#include "vmc_game_object.hpp"

namespace vae {


	class Particle
	{
	public:
		Particle();
		void updatePosition(float dt);

	private:
		TransformComponent transform;

		float mass;
	};

}

