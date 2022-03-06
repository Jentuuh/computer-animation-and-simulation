#include "spline.hpp"

namespace vmc {
	Spline::Spline() 
	{

	}

	void Spline::addControlPoint(glm::vec3 pos, glm::vec3 color, std::shared_ptr<VmcModel> model)
	{
		auto contr_point = VmcGameObject::createGameObject();
		contr_point.model = model;
		contr_point.transform.translation = pos;
		contr_point.transform.scale = { 0.05f, 0.05f, 0.05f };
		contr_point.color = color;

		controlPoints.push_back(std::move(contr_point));
	}

	void Spline::selectNextControlPoint()
	{
		controlPoints[selectedControlPoint].color = { .0f, .0f, 1.0f };
		selectedControlPoint = (selectedControlPoint + 1) % controlPoints.size();
		controlPoints[selectedControlPoint].color = { 1.0f, 1.0f, .0f };
	}

	void Spline::moveCurrentControlPoint(MoveDirection direction, float deltaTime)
	{
		switch (direction)
		{
		case POSX:
			controlPoints[selectedControlPoint].transform.translation.x += deltaTime * pointMovementSpeed;
			break;
		case NEGX:
			controlPoints[selectedControlPoint].transform.translation.x -= deltaTime * pointMovementSpeed;
			break;
		case POSY:
			controlPoints[selectedControlPoint].transform.translation.y -= deltaTime * pointMovementSpeed;
			break;
		case NEGY:
			controlPoints[selectedControlPoint].transform.translation.y += deltaTime * pointMovementSpeed;
			break;

		default:
			break;
		}
		generateSplineSegment(curvePoints[0].model);
	}

	void Spline::generateSplineSegment(std::shared_ptr<VmcModel> curvePointModel)
	{
		curvePoints.clear();
		for (float t = .0f; t < 1.0f; t += 0.01f)
		{
			auto curve_point = VmcGameObject::createGameObject();
			curve_point.model = curvePointModel;
			curve_point.transform.translation = calculateSplinePoint(t);
			curve_point.transform.scale = { 0.05f, 0.05f, 0.05f };
			curve_point.color = { 1.0f, 1.0f, 1.0f };

			curvePoints.push_back(std::move(curve_point));
		}
	}


	glm::vec3 Spline::calculateSplinePoint(float normalizedDist)
	{
		int p0, p1, p2, p3;

		// 4 control point indices
		p1 = (int)normalizedDist + 1;
		p2 = p1 + 1;
		p3 = p2 + 1;
		p0 = p1 - 1;

		// Cache square and cube since we'll use it often
		float dd = pow(normalizedDist, 2);
		float ddd = pow(normalizedDist, 3);

		float q1 = -ddd + 2.0f * dd - normalizedDist;
		float q2 = 3.0f * ddd - 5.0f * dd + 2.0f;
		float q3 = -3.0f * ddd + 4.0f * dd + normalizedDist;
		float q4 = ddd - dd;

		float x_result = 0.5f * (controlPoints[p0].transform.translation.x * q1 + controlPoints[p1].transform.translation.x * q2 + controlPoints[p2].transform.translation.x * q3 + controlPoints[p3].transform.translation.x * q4);
		float y_result = 0.5f * (controlPoints[p0].transform.translation.y * q1 + controlPoints[p1].transform.translation.y * q2 + controlPoints[p2].transform.translation.y * q3 + controlPoints[p3].transform.translation.y * q4);
		float z_result = 0.5f * (controlPoints[p0].transform.translation.z * q1 + controlPoints[p1].transform.translation.z * q2 + controlPoints[p2].transform.translation.z * q3 + controlPoints[p3].transform.translation.z * q4);

		return { x_result, y_result, z_result };
	}

}
