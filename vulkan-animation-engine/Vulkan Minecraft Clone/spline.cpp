#include "spline.hpp"

#include <iostream>

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
		case POSZ:
			controlPoints[selectedControlPoint].transform.translation.z += deltaTime * pointMovementSpeed;
			break;
		case NEGZ:
			controlPoints[selectedControlPoint].transform.translation.z -= deltaTime * pointMovementSpeed;
			break;

		default:
			break;
		}
		generateSplineSegments();
	}

	void Spline::generateSplineSegments()
	{
		// Reset curve points
		curvePoints.clear();
		
		// We set controlPoints.size() - 3.0f as upper limit because we do not want to draw segments at the outer 2 control points
		for (float t = .0f; t < controlPoints.size() - 3.0f; t += 0.01f)
		{
			TransformComponent transform{};
			transform.translation = calculateSplinePoint(t);
			transform.scale = { 0.05f, 0.05f, 0.05f };
			curvePoints.push_back(transform);
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

		// Map distance to [0;1] range to represent the distance of the current segment we're looking at
		normalizedDist = normalizedDist - (int)normalizedDist;

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
