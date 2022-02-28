#pragma once
#include <glm/glm.hpp>

// 3D Spline function
class SplineFunction
{
public:
	SplineFunction(glm::vec3 startPoint, glm::vec3 endPoint);

private:
	// Domain and range of function
	glm::vec3 startPoint;
	glm::vec3 endPoint;

};

