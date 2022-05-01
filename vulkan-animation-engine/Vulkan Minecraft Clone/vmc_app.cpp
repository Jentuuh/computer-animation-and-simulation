#include "vmc_app.hpp"
#include "simple_render_system.hpp"
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
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace vae {

	struct GlobalUbo {
		alignas(16) glm::mat4 projectionView{ 1.0f };
		alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
	};

	VmcApp::VmcApp()
	{
		globalPool = VmcDescriptorPool::Builder(vmcDevice)
			.setMaxSets(VmcSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VmcSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		initDescriptorsAndUBOs();

		initImgui();
		loadGameObjects();
		initLSystems();
		initSkeletons();
		initRigidBodies();
		viewerObject = std::make_unique<VmcGameObject>(VmcGameObject::createGameObject());
	}

	VmcApp::~VmcApp(){
		vkDestroyDescriptorPool(vmcDevice.device(), imGuiPool, nullptr);
	}

	void VmcApp::run()
	{

		SimpleRenderSystem simpleRenderSystem{ 
			vmcDevice, 
			vmcRenderer.getSwapChainRenderPass(), 
			globalSetLayout->getDescriptorSetLayout()};
	
        auto currentTime = std::chrono::high_resolution_clock::now();

		// ImGui state
		bool show_demo_window = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		while (!vmcWindow.shouldClose())
		{
			glfwPollEvents();

			renderImGuiWindow();

            // Time step (delta time)
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, MAX_FRAME_TIME);
			
			updateCamera(frameTime);

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
				simpleRenderSystem.renderGameObjects(
					commandBuffer, 
					globalDescriptorSets[frameIndex], 
					gameObjects, 
					animators, 
					Lsystems[0], 
					skeletons[0], 
					rigidBodies[0], 
					camera, 
					frameTime,
					sphereModel,
					cameraMode,
					viewerObject.get());
				vmcRenderer.endSwapChainRenderPass(commandBuffer);

				// Draw ImGui stuff
				vmcRenderer.beginImGuiRenderPass(commandBuffer);
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
				vmcRenderer.endImGuiRenderPass(commandBuffer);

				vmcRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(vmcDevice.device());
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}


	void VmcApp::initImgui()
	{
		// Create descriptor pool for ImGui
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;


		if (vkCreateDescriptorPool(vmcDevice.device(), &pool_info, nullptr, &imGuiPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool for imgui!");
		}

		// Init ImGui library
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// ImGui style
		ImGui::StyleColorsDark();

		// Platform/renderer bindings
		ImGui_ImplGlfw_InitForVulkan(vmcWindow.getGLFWwindow(), true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vmcDevice.getInstance();
		init_info.PhysicalDevice = vmcDevice.getPhysicalDevice();
		init_info.Device = vmcDevice.device();
		init_info.Queue = vmcDevice.graphicsQueue();
		init_info.DescriptorPool = imGuiPool;
		init_info.MinImageCount = VmcSwapChain::MAX_FRAMES_IN_FLIGHT;
		init_info.ImageCount = VmcSwapChain::MAX_FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		// ImGui Vulkan initialization
		ImGui_ImplVulkan_Init(&init_info, vmcRenderer.getImGuiRenderPass());

		// Upload fonts to GPU
		VkCommandBuffer command_buffer = vmcDevice.beginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
		vmcDevice.endSingleTimeCommands(command_buffer);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

 
	void VmcApp::loadGameObjects()
    {
		// Stick figure
		std::shared_ptr<VmcModel> stickModel = VmcModel::createModelFromFile(vmcDevice, "../Models/stick_fig/body_stick.obj");
		std::shared_ptr<VmcModel> armModel = VmcModel::createModelFromFile(vmcDevice, "../Models/stick_fig/arm_left.obj");

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

	void VmcApp::initDescriptorsAndUBOs()
	{
		uboBuffers.resize(VmcSwapChain::MAX_FRAMES_IN_FLIGHT);
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

		 globalSetLayout = std::move(VmcDescriptorSetLayout::Builder(vmcDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build());

	globalDescriptorSets.resize(VmcSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			// Write buffer info to binding 0
			VmcDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}
	}


	void VmcApp::renderImGuiWindow()
	{
		const char* controlPoints[] = { "1", };
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Vulkan Animation Engine UI");
		ImGui::TextWrapped("This UI window allows you to manage the scene content.");
		
		ImGui::Text("Camera Mode:");
		ImGui::RadioButton("Edit mode", &cameraMode, 0); ImGui::SameLine();
		ImGui::RadioButton("Free roam mode", &cameraMode, 1); ImGui::SameLine();
		ImGui::RadioButton("Animator mode", &cameraMode, 2);

		//if (ImGui::Checkbox("Edit mode", &editMode))
		//{
		//	if (editMode)
		//		viewerObject->transform.translation = { 0.0f, 0.0f, 0.0f };
		//};

		// =====================
		// SPLINE ANIMATOR UI
		// =====================
		if (ImGui::Button("Add Spline Curve Animator"))
		{
			addSplineAnimator();
			animators.back().addAnimatedObject(&gameObjects[0]);
		}

		for (int i = 0; i < animators.size(); i++)
		{

			ImGui::TextWrapped("==============================================");
			std::string animatorTitle = "Spline animator ";
			ImGui::TextWrapped((animatorTitle + std::to_string(i)).c_str());

			ImGui::InputFloat("Duration", &animators[i].getTotalTime(), 0.0f, 0.0f, "%.3f s");

			std::string removeLabel = "Remove animator ";
			if (ImGui::Button((removeLabel + std::to_string(i)).c_str()))
			{
				animators.erase(animators.begin() + i);
			}

			std::string animatorMoveLabel = "Pos anim ";
			if (ImGui::DragFloat3((animatorMoveLabel + std::to_string(i)).c_str(), glm::value_ptr(animators[i].getPosition()), 1.0f, -20.0f, 20.0f))
			{
				animators[i].updateControlAndCurvePoints();
				animators[i].getSpline().generateSplineSegments();
				animators[i].buildForwardDifferencingTable();
			}
			
			std::string addCPLabel = "ADD CP (anim ";
			if (ImGui::Button((addCPLabel + std::to_string(i) + ")").c_str()))
			{
				animators[i].addControlPoint(ControlPoint{ {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.1f}, sphereModel }, animators[i].getPosition());
			}
		
			std::vector<VmcGameObject>& CPs = animators[i].getControlPoints();
			for(int j = 0; j < CPs.size(); j++)
			{
				std::string cpLabel = "CP ";
				if (ImGui::DragFloat3((cpLabel + std::to_string(i) + "." + std::to_string(j)).c_str(), glm::value_ptr(CPs[j].transform.translation), 1.0f, -50.0f, 50.0f)) {
					animators[i].getSpline().generateSplineSegments();
					animators[i].buildForwardDifferencingTable();
				};

				ImGui::SameLine();
				std::string removeCPLabel = "DEL (";
				
				if (ImGui::Button((removeCPLabel + std::to_string(i) + "." + std::to_string(j) + ")").c_str()))
				{
					animators[i].removeControlPoint(j);
				}
			}
			ImGui::TextWrapped("==============================================");
		}

		ImGui::End();

		ImGui::Render();
	}

	void VmcApp::addSplineAnimator()
	{
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

		SplineAnimator splineAnimator{ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, controlPoints, 4.0f };
		splineAnimator.getSpline().generateSplineSegments();
		animators.push_back(std::move(splineAnimator));

		// Build forward differencing table based on curve points
		animators.back().buildForwardDifferencingTable();
		animators.back().printForwardDifferencingTable();
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

	void VmcApp::updateCamera(float frameTime)
	{
		float aspect = vmcRenderer.getAspectRatio();

		switch (cameraMode)
		{
		// EDIT MODE
		case 0:
			viewerObject->transform.translation = { 10.0f, -10.0f, -10.0f };
			camera.setViewTarget(viewerObject->transform.translation, { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f });
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
			break;
		
		// ROAM MODE
		case 1:
			// Update camera model (game object that contains camera
			cameraController.moveInPlaneXZ(vmcWindow.getGLFWwindow(), frameTime, *viewerObject);
			// Update camera view matrix
			camera.setViewYXZ(viewerObject->transform.translation, viewerObject->transform.rotation);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
			break;

		// ANIMATOR MODE
		case 2:
			// Update camera view matrix (translation is handled by the animator)
			camera.setViewYXZ(viewerObject->transform.translation, viewerObject->transform.rotation);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
			break;
		default:
			break;
		}
	}
}