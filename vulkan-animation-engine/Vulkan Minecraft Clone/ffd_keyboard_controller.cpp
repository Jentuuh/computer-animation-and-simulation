#include "ffd_keyboard_controller.hpp"

namespace vmc {
	void FFDKeyboardController::updateDeformationGrid(GLFWwindow* window, float dt, FFD& deformationSys)
	{
		if (glfwGetKey(window, keys.moveXPos) == GLFW_PRESS) {
			deformationSys.moveCurrentControlPoint(POSX, dt);
		}
		if (glfwGetKey(window, keys.moveXNeg) == GLFW_PRESS) {
			deformationSys.moveCurrentControlPoint(NEGX, dt);
		}
		if (glfwGetKey(window, keys.moveYPos) == GLFW_PRESS) {
			deformationSys.moveCurrentControlPoint(POSY, dt);
		}
		if (glfwGetKey(window, keys.moveYNeg) == GLFW_PRESS) {
			deformationSys.moveCurrentControlPoint(NEGY, dt);
		}
		if (glfwGetKey(window, keys.moveZPos) == GLFW_PRESS) {
			deformationSys.moveCurrentControlPoint(POSZ, dt);
		}
		if (glfwGetKey(window, keys.moveZNeg) == GLFW_PRESS) {
			deformationSys.moveCurrentControlPoint(NEGZ, dt);
		}

		// Next
		if (glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS && !pressedNext)
		{
			deformationSys.selectNextControlPoint();
			pressedNext = true;
		}

		// Previous
		if (glfwGetKey(window, keys.selectPrevCP) == GLFW_PRESS && !pressedNext)
		{
			deformationSys.selectPrevControlPoint();
			pressedNext = true;
		}
		
		// Spam prevention
		if (!glfwGetKey(window, keys.selectPrevCP) == GLFW_PRESS && !glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS) {
			pressedNext = false;
		}

	}

}