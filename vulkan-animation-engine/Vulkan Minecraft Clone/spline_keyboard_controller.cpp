#include "spline_keyboard_controller.hpp"

namespace vmc {


	void SplineKeyboardController::updateSpine(GLFWwindow* window, float dt, Spline& spline)
	{
		if (glfwGetKey(window, keys.moveXPos) == GLFW_PRESS) spline.moveCurrentControlPoint(POSX, dt);
		if (glfwGetKey(window, keys.moveXNeg) == GLFW_PRESS) spline.moveCurrentControlPoint(NEGX, dt);
		if (glfwGetKey(window, keys.moveYPos) == GLFW_PRESS) spline.moveCurrentControlPoint(POSY, dt);
		if (glfwGetKey(window, keys.moveYNeg) == GLFW_PRESS) spline.moveCurrentControlPoint(NEGY, dt);
		if (glfwGetKey(window, keys.moveZPos) == GLFW_PRESS) spline.moveCurrentControlPoint(POSZ, dt);
		if (glfwGetKey(window, keys.moveZNeg) == GLFW_PRESS) spline.moveCurrentControlPoint(NEGZ, dt);

		if (glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS && !pressedNext)
		{
			spline.selectNextControlPoint();
			pressedNext = true;
		}
		else if(!glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS) {
			pressedNext = false;
		}
	}
}