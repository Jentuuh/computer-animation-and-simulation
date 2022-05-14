#include "animatable.hpp"

namespace vae {
	Animatable::Animatable(float startTime, float duration):startTime{startTime}, duration{duration}{}

	void Animatable::advanceTime(float dt)
	{
		timePassed += dt;
	}
}