#include "vmc_app.hpp"
#include "simple_render_system.hpp"
#include "vmc_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "spline_keyboard_controller.hpp"

// std
#include <cassert>
#include <stdexcept>
#include <array>
#include <chrono>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include<glm/gtc/constants.hpp>

namespace vmc {

	VmcApp::VmcApp()
	{
		loadGameObjects();
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
			
			splineController.updateSpine(vmcWindow.getGLFWwindow(), frameTime, animator.splineCurve);

			// Render loop
			if (auto commandBuffer = vmcRenderer.beginFrame()) {
				vmcRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, animator, camera, frameTime);
				vmcRenderer.endSwapChainRenderPass(commandBuffer);
				vmcRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(vmcDevice.device());
	}

 
	void VmcApp::loadGameObjects()
    {
		// Cube
	/*	std::shared_ptr<VmcModel> vmcModel = VmcModel::createModelFromFile(vmcDevice, "../Models/cube.obj");

		auto gameObj = VmcGameObject::createGameObject();
		gameObj.models["Main"] = vmcModel;
		gameObj.transform.translation = { .0f, .0f, 2.5f };
		gameObj.transform.scale = { .3f, .3f, .3f };
		gameObjects.push_back(std::move(gameObj));*/

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
	

		// Chunk object
		//auto chunkObj = VmcGameObject::createGameObject();
		//std::shared_ptr<VmcModel> chunkModel = VmcModel::createChunkModelMesh(vmcDevice, chunkObj.chunk);
		//chunkObj.model = chunkModel;
		//chunkObj.transform.translation = { 2.0f, .0f, .0f };
		//chunkObj.transform.scale = { .5f, .5f, .5f };
		//gameObjects.push_back(std::move(chunkObj));

		// Initialize animator and animation curve
		std::shared_ptr<VmcModel> sphereModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");
		float delta_x = 0.1;
		float x = 0.0f;
		// Control points for animation path
		for (int i = 0; i < 200; i++) {
			animator.addControlPoint({ 0.0f, glm::sin(x), x }, { 1.0f, 0.0f, 0.0f }, sphereModel);
			x += delta_x;
		}
		animator.buildForwardDifferencingTable();
		animator.printForwardDifferencingTable();

		// Init spline + Spline control points
		animator.splineCurve.addControlPoint({ 0.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel);
		animator.splineCurve.addControlPoint({ 1.0f, 1.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel);
		animator.splineCurve.addControlPoint({ 2.0f, 1.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel);
		animator.splineCurve.addControlPoint({ 3.0f, 3.0f, 2.5f }, { 0.0f, 0.0f, 1.0f }, sphereModel);
		animator.splineCurve.generateSplineSegment(sphereModel);
	}
}