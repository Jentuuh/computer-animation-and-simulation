#pragma once
#include "vmc_window.hpp"
#include "vmc_game_object.hpp"
#include "spline.hpp"
#include "animator.hpp"


namespace vmc {
	class SplineKeyboardController
	{
	public:
        struct SplineMovementKeyMappings {
            int moveXPos = GLFW_KEY_L;
            int moveXNeg = GLFW_KEY_J;
            int moveYPos = GLFW_KEY_I;
            int moveYNeg = GLFW_KEY_M;
            int moveZPos = GLFW_KEY_O;
            int moveZNeg = GLFW_KEY_P;
            int selectNextCP = GLFW_KEY_ENTER;
        };

        void updateSpline(GLFWwindow* window, float dt, Animator& animator);

        SplineMovementKeyMappings keys{};
        bool pressedNext = false;
	};

}

