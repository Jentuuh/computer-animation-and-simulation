#include "story_board.hpp"
#include <iostream>

namespace vae {
	StoryBoard::StoryBoard() {}

	void StoryBoard::addAnimatable(Animatable* newAnimatable)
	{
		animatables.push_back(newAnimatable);
		updateStoryBoardDuration();
	}

	void StoryBoard::removeAnimatable(int index)
	{
		animatables.erase(animatables.begin() + index);
		updateStoryBoardDuration();
	}

	void StoryBoard::removeAnimatable(Animatable* animatableToRemove)
	{
		int index = 0;
		for (auto a : animatables)
		{
			if (a == animatableToRemove)
				break;
			index++;
		}
		animatables.erase(animatables.begin() + index);
		updateStoryBoardDuration();
	}

	void StoryBoard::startStoryBoardAnimation()
	{
		timePassed = 0.0f;
		animationRunning = true;
	}

	void StoryBoard::updateAnimatables(float dt)
	{
		if (!animationRunning)
			return;

		timePassed += dt;

		// Update all animatables
		for (auto a : animatables)
		{
			a->animateAnimatable(timePassed);
		}

		// Check if storyboard animation is completed
		if (timePassed >= storyBoardDuration)
		{
			// Animatable cleanup
			for (auto a : animatables)
			{
				a->cleanUpAnimatable();
			}
			animationRunning = false;
			timePassed = 0;
		}
	}

	void StoryBoard::updateStoryBoardDuration()
	{
		float max = 0.0f;
		for (auto a : animatables)
		{
			if (a->getEndTime() > max)
			{
				max = a->getEndTime();
			}
		}

		storyBoardDuration = max;
	}

	bool StoryBoard::containsAnimatable(Animatable* animatable)
	{
		for (auto a : animatables)
		{
			if (a == animatable)
				return true;
		}
		return false;
	}
}