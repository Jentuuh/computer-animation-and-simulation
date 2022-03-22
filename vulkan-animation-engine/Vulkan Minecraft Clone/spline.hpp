#pragma once
#include "vmc_game_object.hpp"
#include "enums.hpp"

// glm
#include <glm/glm.hpp>

// std
#include <vector>



// 3D Catmull Rom Spline with movable control points
namespace vmc {

	class Spline
	{
	public:
		Spline();

		int getSelectedCpIndex() { return selectedControlPoint; };
		std::vector<VmcGameObject>& getControlPoints() { return controlPoints; };
		std::vector<TransformComponent>& getCurvePoints() { return curvePoints; };

		void addControlPoint(glm::vec3 pos, glm::vec3 color, std::shared_ptr<VmcModel> model);
		void moveCurrentControlPoint(MoveDirection direction, float deltaTime);
		void selectNextControlPoint();

		void generateSplineSegments();

	private:
		glm::vec3 calculateSplinePoint(float normalizedDist);

		// Control points defining the spline
		std::vector<VmcGameObject> controlPoints;
		std::vector<TransformComponent> curvePoints;
		int selectedControlPoint = 0;
		float pointMovementSpeed = 5.0f;
	};


}
