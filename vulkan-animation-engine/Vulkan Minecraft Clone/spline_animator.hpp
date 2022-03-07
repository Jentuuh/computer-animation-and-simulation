#pragma once
#include "spline.hpp"

namespace vmc {
	class SplineAnimator
	{
	public:
		SplineAnimator();

		Spline& getSpline() { return splineCurve; };

		void advanceTime(float deltaTime);


	private:
		Spline splineCurve;
	};
}
