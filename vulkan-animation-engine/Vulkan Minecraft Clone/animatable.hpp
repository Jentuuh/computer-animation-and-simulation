#pragma once

namespace vae {
	class Animatable
	{
	public:
		Animatable(float startTime, float duration);
		float getEndTime() { return startTime + duration; };
		float getTimePassed() { return timePassed; };
		float& getAnimationDuration() { return duration; };

		void advanceTime(float dt);
		virtual void updateAnimatable() = 0;

	protected:
		float startTime;
		float duration;
		float timePassed = 0.0f;
	};
}


