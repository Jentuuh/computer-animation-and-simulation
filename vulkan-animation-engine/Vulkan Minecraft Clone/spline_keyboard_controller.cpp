#include "spline_keyboard_controller.hpp"

namespace vmc {


	void SplineKeyboardController::updateSpine(GLFWwindow* window, float dt, SplineAnimator& animator)
	{
		if (glfwGetKey(window, keys.moveXPos) == GLFW_PRESS){
			animator.getSpline().moveCurrentControlPoint(POSX, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveXNeg) == GLFW_PRESS) {
			animator.getSpline().moveCurrentControlPoint(NEGX, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveYPos) == GLFW_PRESS) {
			animator.getSpline().moveCurrentControlPoint(POSY, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveYNeg) == GLFW_PRESS) {
			animator.getSpline().moveCurrentControlPoint(NEGY, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveZPos) == GLFW_PRESS) {
			animator.getSpline().moveCurrentControlPoint(POSZ, dt);
			animator.buildForwardDifferencingTable();
		}
		if (glfwGetKey(window, keys.moveZNeg) == GLFW_PRESS) {
			animator.getSpline().moveCurrentControlPoint(NEGZ, dt);
			animator.buildForwardDifferencingTable();
		}

		if (glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS && !pressedNext)
		{
			animator.getSpline().selectNextControlPoint();
			pressedNext = true;
		}
		else if(!glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS) {
			pressedNext = false;
		}
	}
}