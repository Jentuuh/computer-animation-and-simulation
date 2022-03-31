#include "ffd_keyboard_controller.hpp"

namespace vmc {
	void FFDKeyboardController::updateDeformationGrid(GLFWwindow* window, float dt, VmcGameObject& deformingObject)
	{
		if (glfwGetKey(window, keys.moveXPos) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(POSX, dt);
			deformingObject.deformObject();
		}
		if (glfwGetKey(window, keys.moveXNeg) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(NEGX, dt);
			deformingObject.deformObject();
		}
		if (glfwGetKey(window, keys.moveYPos) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(POSY, dt);
			deformingObject.deformObject();
		}
		if (glfwGetKey(window, keys.moveYNeg) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(NEGY, dt);
			deformingObject.deformObject();
		}
		if (glfwGetKey(window, keys.moveZPos) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(POSZ, dt);
			deformingObject.deformObject();
		}
		if (glfwGetKey(window, keys.moveZNeg) == GLFW_PRESS) {
			deformingObject.deformationSystem.moveCurrentControlPoint(NEGZ, dt);
			deformingObject.deformObject();
		}

		// Update Object form
		if (glfwGetKey(window, keys.updateObject) == GLFW_PRESS && !pressedUpdate)
		{
			deformingObject.confirmObjectDeformation();
			deformingObject.deformationSystem.resetControlPoints();
			pressedUpdate = true;
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
		if (!glfwGetKey(window, keys.updateObject) == GLFW_PRESS) {
			pressedUpdate = false;
		}
	}

}