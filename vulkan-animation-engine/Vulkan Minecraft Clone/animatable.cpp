#include "animatable.hpp"

namespace vae {
	Animatable::Animatable(float startTime, float duration):startTime{startTime}, duration{duration}{}

	void Animatable::animateAnimatable(float currentStoryBoardTime)
	{
		if (currentStoryBoardTime < startTime)
			return;
		timePassed = currentStoryBoardTime - startTime;
		if (timePassed > duration)
			return;
		updateAnimatable();
	}

}