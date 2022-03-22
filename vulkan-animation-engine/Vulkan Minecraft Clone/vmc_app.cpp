#include "vmc_app.hpp"
#include "simple_render_system.hpp"
#include "vmc_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "spline_keyboard_controller.hpp"
#include "ffd_keyboard_controller.hpp"
#include "spline_animator.hpp"

// std
#include <cassert>
#include <stdexcept>
#include <array>
#include <iostream>
#include <chrono>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>

namespace vmc {

	VmcApp::VmcApp()
	{
		loadGameObjects();
		initLSystems();
		initDeformationSystems();
	}

	VmcApp::~VmcApp()
	{
	}

	void VmcApp::run()
	{
		SimpleRenderSystem simpleRenderSystem{ vmcDevice, vmcRenderer.getSwapChainRenderPass() };
		VmcCamera camera{};

		// Camera controller + camera state container (camera game object) --> WON'T BE RENDERED!
		// The camera's game object only manages the state (rotation + translation) of the camera.
		auto viewerObject = VmcGameObject::createGameObject();
		KeyboardMovementController cameraController{};
		SplineKeyboardController splineController{};
		FFDKeyboardController ffdController{};

		// Initialize animators
		std::shared_ptr<VmcModel> sphereModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");
		std::vector<ControlPoint> controlPoints{};
		controlPoints.push_back({ { 0.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });
		controlPoints.push_back({ { 1.0f, 1.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });
		controlPoints.push_back({ { 2.0f, 1.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });
		controlPoints.push_back({ { 3.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });
		controlPoints.push_back({ { 4.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });
		controlPoints.push_back({ { 5.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });
		controlPoints.push_back({ { 6.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });
		controlPoints.push_back({ { 7.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel });

		SplineAnimator splineAnimator{controlPoints};
		splineAnimator.getSpline().generateSplineSegments();
		animators.push_back(std::move(splineAnimator));

		// Build forward differencing table based on curve points
		animators[0].buildForwardDifferencingTable();
		animators[0].printForwardDifferencingTable();

        auto currentTime = std::chrono::high_resolution_clock::now();

		while (!vmcWindow.shouldClose())
		{
			glfwPollEvents();

            // Time step (delta time)
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, MAX_FRAME_TIME);

			// Update camera model (game object that contains camera
            cameraController.moveInPlaneXZ(vmcWindow.getGLFWwindow(), frameTime, viewerObject);
			// Update camera view matrix
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = vmcRenderer.getAspectRatio();

            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
			
			// Controllers
			splineController.updateSpline(vmcWindow.getGLFWwindow(), frameTime, animators[0]);
			ffdController.updateDeformationGrid(vmcWindow.getGLFWwindow(), frameTime, deformationSystems[0]);

			// Generate L-system iterations
			if (glfwGetKey(vmcWindow.getGLFWwindow(), GLFW_KEY_C) == GLFW_PRESS)
			{
				Lsystems[0].iterate();
			}

			// Render loop
			if (auto commandBuffer = vmcRenderer.beginFrame()) {
				vmcRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, animators[0], Lsystems[0], deformationSystems[0], camera, frameTime);
				vmcRenderer.endSwapChainRenderPass(commandBuffer);
				vmcRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(vmcDevice.device());
	}

 
	void VmcApp::loadGameObjects()
    {
		// Stick figure
		std::shared_ptr<VmcModel> stickModel = VmcModel::createModelFromFile(vmcDevice, "../Models/stick_fig/body_stick.obj");
		std::shared_ptr<VmcModel> armModel = VmcModel::createModelFromFile(vmcDevice, "../Models/stick_fig/arm_left.obj");

		auto stickObj = VmcGameObject::createGameObject();
		stickObj.model = stickModel;
		stickObj.transform.translation = { .0f, .0f, 2.5f };
		stickObj.transform.rotation = { .0f, .0f, -glm::pi<float>() };
		stickObj.transform.scale = { .3f, .3f, .3f };

		auto armObj = VmcGameObject::createGameObject();
		armObj.model = armModel;
		armObj.transform.translation = { .0f, .0f, 2.5f };
		armObj.transform.rotation = { .0f, .0f, -glm::pi<float>() };
		armObj.transform.scale = { .3f, .3f, .3f };
		stickObj.addChild(&armObj);

		gameObjects.push_back(std::move(stickObj));

		// Initialize animator and animation curve
		//std::shared_ptr<VmcModel> sphereModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");
		//float delta_x = 0.1;
		//float x = 0.0f;
		//// Control points for animation path
		//for (int i = 0; i < 200; i++) {
		//	animator.addControlPoint({ 0.0f, glm::sin(x), x }, { 1.0f, 0.0f, 0.0f }, sphereModel);
		//	x += delta_x;
		//}
	}

	void VmcApp::initLSystems() 
	{
		std::vector<std::pair<std::string, float>> rules1;
		rules1.push_back(std::make_pair("F=>F[+F]F[-F][F]", 0.25f));
		rules1.push_back(std::make_pair("F=>F[+F]F[-F]F", 0.25f));
		rules1.push_back(std::make_pair("F=>FFF", 0.5f));

		std::string axiom1 = "F";
		Lsystems.push_back(LSystem{ rules1, axiom1, {0.0f, 0.0f, 0.0f}, 5, 25.7f });
	}

	void VmcApp::initDeformationSystems()
	{
		FFDInitializer FFDCreateInfo{};
		FFDCreateInfo.startX = 0.0f;
		FFDCreateInfo.endX = 5.0f;
		FFDCreateInfo.startY = 0.0f;
		FFDCreateInfo.endY = -2.0f;
		FFDCreateInfo.startZ = 0.0f;
		FFDCreateInfo.endZ = 2.0f;

		FFDCreateInfo.resX = 5.0f;
		FFDCreateInfo.resY = 2.0f;
		FFDCreateInfo.resZ = 2.0f;

		deformationSystems.push_back(FFD{ FFDCreateInfo });
	}
}