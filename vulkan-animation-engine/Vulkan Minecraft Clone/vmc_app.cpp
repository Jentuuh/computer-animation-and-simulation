#include "vmc_app.hpp"
#include "simple_render_system.hpp"
#include "vmc_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "spline_keyboard_controller.hpp"
#include "ffd_keyboard_controller.hpp"
#include "spline_animator.hpp"
#include "vmc_buffer.hpp"

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

namespace vae {

	struct GlobalUbo {
		glm::mat4 projectionView{ 1.0f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
	};

	VmcApp::VmcApp()
	{
		loadGameObjects();
		initLSystems();
		initSkeletons();
		initRigidBodies();
	}

	VmcApp::~VmcApp()
	{
	}

	void VmcApp::run()
	{

		std::vector<std::unique_ptr<VmcBuffer>> uboBuffers(VmcSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++)
		{
			uboBuffers[i] = std::make_unique<VmcBuffer>(
				vmcDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			uboBuffers[i]->map();
		}

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
			ffdController.updateDeformationGrid(vmcWindow.getGLFWwindow(), frameTime, gameObjects[1]);

			// Generate L-system iterations
			if (glfwGetKey(vmcWindow.getGLFWwindow(), GLFW_KEY_C) == GLFW_PRESS)
			{
				Lsystems[0].iterate();
			}

			// Update rigid bodies
			rigidBodies[0].updateState(frameTime);

			// Render loop
			if (auto commandBuffer = vmcRenderer.beginFrame()) {
				int frameIndex = vmcRenderer.getFrameIndex();
				
				// Update phase
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// Render phase
				vmcRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, animators[0], Lsystems[0], skeletons[0], rigidBodies[0], camera, frameTime);
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
		std::shared_ptr<VmcModel> sphereModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");

		auto stickObj = VmcGameObject::createGameObject();
		stickObj.model = stickModel;
		// Make sure to call initDeformationSystem() AFTER the model is assigned to the gameObj!
		//stickObj.initDeformationSystem();
		stickObj.setPosition({ .0f, .0f, .0f });
		stickObj.transform.rotation = { .0f, .0f, -glm::pi<float>() };
		stickObj.setScale({ .3f, .3f, .3f });

		auto armObj = VmcGameObject::createGameObject();
		armObj.model = armModel;
		armObj.transform.translation = { .0f, .0f, 2.5f };
		armObj.transform.rotation = { .0f, .0f, -glm::pi<float>() };
		armObj.transform.scale = { .3f, .3f, .3f };
		stickObj.addChild(&armObj);

		gameObjects.push_back(std::move(stickObj));

		auto sphere = VmcGameObject::createGameObject();
		sphere.model = sphereModel;
		sphere.initDeformationSystem();
		sphere.setPosition({ .0f, .0f, .0f });
		sphere.transform.rotation = { .0f, .0f, .0f };
		sphere.setScale({ 1.0f, 1.0f, 1.0f });

		gameObjects.push_back(std::move(sphere));


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

	void VmcApp::initSkeletons()
	{
		std::shared_ptr<VmcModel> jointModel = VmcModel::createModelFromFile(vmcDevice, "../Models/joint.obj");
		std::shared_ptr<VmcModel> linkModel = VmcModel::createModelFromFile(vmcDevice, "../Models/stick_fig/arm_left.obj");

		Skeleton skeleton{linkModel, jointModel};
		skeletons.push_back(skeleton);
	}

	void VmcApp::initRigidBodies()
	{
		std::vector<std::pair<glm::vec3, float>> massPoints;
		massPoints.push_back(std::make_pair(glm::vec3{ 1.0f, 0.0f, 0.0f }, 1.0f));
		massPoints.push_back(std::make_pair(glm::vec3{ 0.0f, 1.0f, 0.0f }, 1.0f));
		massPoints.push_back(std::make_pair(glm::vec3{ 0.0f, 0.0f, 1.0f }, 1.0f));

		std::shared_ptr<VmcModel> rigidModel = VmcModel::createModelFromFile(vmcDevice, "../Models/cube.obj");

		RigidBody rigid_1{massPoints, false, rigidModel};
		rigid_1.applyTorque({ 1.0f, .0f, 1.0f });
		rigidBodies.push_back(rigid_1);
	}

}