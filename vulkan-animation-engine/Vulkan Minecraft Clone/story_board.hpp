#pragma once
#include "animatable.hpp"
#include "spline_animator.hpp"

#include <vector>

namespace vae {
	class StoryBoard
	{
	public:
		StoryBoard();

		void addAnimatable(Animatable* newAnimatable);
		void removeAnimatable(int index);
		void removeAnimatable(Animatable* animatableToRemove);

		void startStoryBoardAnimation();

		void updateAnimatables(float dt);
		void updateStoryBoardDuration();

		bool containsAnimatable(Animatable* animatable);

		std::vector<Animatable*> animatables;
	private:
		bool animationRunning;
		float timePassed;
		float storyBoardDuration;
	};

}

