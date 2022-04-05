#include "spline_keyboard_controller.hpp"

#include <iostream>

namespace vae {


	void SplineKeyboardController::updateSpline(GLFWwindow* window, float dt, Animator& animator)
	{
		if (glfwGetKey(window, keys.moveXPos) == GLFW_PRESS){
			animator.moveCurrentControlPoint(POSX, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveXNeg) == GLFW_PRESS) {
			animator.moveCurrentControlPoint(NEGX, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveYPos) == GLFW_PRESS) {
			animator.moveCurrentControlPoint(POSY, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveYNeg) == GLFW_PRESS) {
			animator.moveCurrentControlPoint(NEGY, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveZPos) == GLFW_PRESS) {
			animator.moveCurrentControlPoint(POSZ, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveZNeg) == GLFW_PRESS) {
			animator.moveCurrentControlPoint(NEGZ, dt);
			animator.buildForwardDifferencingTable();
		}

		if (glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS && !pressedNext)
		{
			animator.selectNextControlPoint();
			pressedNext = true;
		}
		else if(!glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS) {
			pressedNext = false;
		}
	}
}