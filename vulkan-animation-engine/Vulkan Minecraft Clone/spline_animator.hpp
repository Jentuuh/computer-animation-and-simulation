#pragma once
#include "spline.hpp"
#include "vmc_game_object.hpp"
#include "animator.hpp"

namespace vmc {
	struct ControlPoint {
		glm::vec3 pos;
		glm::vec3 col;
		std::shared_ptr<VmcModel> model;
	};

	class SplineAnimator: public Animator
	{
	public:
		SplineAnimator(std::vector<ControlPoint> controlPoints);

		Spline& getSpline() { return splineCurve; };

		glm::vec3 calculateNextPositionSpeedControlled();
		glm::vec3 calculateNextRotationParabolic();
		std::vector<TransformComponent>& getCurvePoints();
		std::vector<VmcGameObject>& getControlPoints();
		void moveCurrentControlPoint(MoveDirection d, float dt);
		void selectNextControlPoint();

	private:
		Spline splineCurve;
	};
}
