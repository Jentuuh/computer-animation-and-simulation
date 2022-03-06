#pragma once
#include "vmc_game_object.hpp"

// glm
#include <glm/glm.hpp>

// std
#include <vector>



// 3D Catmull Rom Spline
namespace vmc {

	enum MoveDirection {POSX, NEGX, POSY, NEGY};

	class Spline
	{
	public:
		Spline();

		int getSelectedCpIndex() { return selectedControlPoint; };
		std::vector<VmcGameObject>& getControlPoints() { return controlPoints; };
		std::vector<VmcGameObject>& getCurvePoints() { return curvePoints; };

		void addControlPoint(glm::vec3 pos, glm::vec3 color, std::shared_ptr<VmcModel> model);
		void moveCurrentControlPoint(MoveDirection direction, float deltaTime);
		void selectNextControlPoint();

		void generateSplineSegment(std::shared_ptr<VmcModel> curvePointModel);


	private:
		glm::vec3 calculateSplinePoint(float normalizedDist);

		// Control points defining the spline
		std::vector<VmcGameObject> controlPoints;
		std::vector<VmcGameObject> curvePoints;
		int selectedControlPoint = 0;
		float pointMovementSpeed = 5.0f;
	};


}
