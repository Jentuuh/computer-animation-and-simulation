#pragma once
#include "spline.hpp"
#include "vmc_game_object.hpp"
#include "animator.hpp"

#include <glm/glm.hpp>

namespace vae {
	struct ControlPoint {
		glm::vec3 pos;
		glm::vec3 col;
		std::shared_ptr<VmcModel> model;
	};

	class SplineAnimator: public Animator
	{
	public:
		SplineAnimator(glm::vec3 pos, glm::vec3 startOrientation, glm::vec3 endOrientation, std::vector<ControlPoint> controlPoints, float animationTime, float startTime);

		Spline& getSpline() { return splineCurve; };
		std::vector<TransformComponent>& getCurvePoints();
		std::vector<VmcGameObject>& getControlPoints();

		glm::vec3 calculateNextPositionSpeedControlled();
		glm::vec3 calculateIntermediateRotation();
		void updateControlAndCurvePoints();

		void addControlPoint(ControlPoint newControlPoint, glm::vec3 offset);
		void removeControlPoint(int index);
		void moveCurrentControlPoint(MoveDirection d, float dt);
		void selectNextControlPoint();

	private:
		Spline splineCurve;
	};
}
