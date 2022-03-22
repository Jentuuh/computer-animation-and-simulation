#pragma once
#include "vmc_window.hpp"
#include "vmc_game_object.hpp"
#include "ffd.hpp"

namespace vmc {
	class FFDKeyboardController
    {
    public:
        struct FFDGridCPMovementKeyMappings {
            int moveXPos = GLFW_KEY_1;
            int moveXNeg = GLFW_KEY_2;
            int moveYPos = GLFW_KEY_3;
            int moveYNeg = GLFW_KEY_4;
            int moveZPos = GLFW_KEY_5;
            int moveZNeg = GLFW_KEY_6;
            int selectNextCP = GLFW_KEY_TAB;
            int selectPrevCP = GLFW_KEY_LEFT_CONTROL;
        };

        void updateDeformationGrid(GLFWwindow* window, float dt, FFD& deformationSys);

        FFDGridCPMovementKeyMappings keys{};
        bool pressedNext = false;
    };
}


