#include "ffd_keyboard_controller.hpp"

namespace vmc {
	void FFDKeyboardController::updateDeformationGrid(GLFWwindow* window, float dt, VmcGameObject& deformingObject)
	{
		if (glfwGetKey(window, keys.moveXPos) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(POSX, dt);
		}
		if (glfwGetKey(window, keys.moveXNeg) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(NEGX, dt);
		}
		if (glfwGetKey(window, keys.moveYPos) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(POSY, dt);
		}
		if (glfwGetKey(window, keys.moveYNeg) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(NEGY, dt);
		}
		if (glfwGetKey(window, keys.moveZPos) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(POSZ, dt);
		}
		if (glfwGetKey(window, keys.moveZNeg) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(NEGZ, dt);
		}

		// Next
		if (glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS && !pressedNext)
		{
			deformingObject.deformationSystem.selectNextControlPoint();
			pressedNext = true;
		}

		// Previous
		if (glfwGetKey(window, keys.selectPrevCP) == GLFW_PRESS && !pressedNext)
		{
			deformingObject.deformationSystem.selectPrevControlPoint();
			pressedNext = true;
		}
		
		// Spam prevention
		if (!glfwGetKey(window, keys.selectPrevCP) == GLFW_PRESS && !glfwGetKey(window, keys.selectNextCP) == GLFW_PRESS) {
			pressedNext = false;
		}

	}

}