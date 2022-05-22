#include "vmc_app.hpp"
#include "spline_animator.hpp"
#include "vmc_buffer.hpp"

// std
#include <cassert>
#include <stdexcept>
#include <array>
#include <iostream>
#include <chrono>
#include <filesystem>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace vae {

	struct GlobalUbo {
		alignas(16) glm::mat4 projection{ 1.0f };
		alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
		alignas(16) glm::mat4 view{ 1.0f };
	};

	VmcApp::VmcApp()
	{
		loadTextures();

		globalPool = VmcDescriptorPool::Builder(vmcDevice)
			.setMaxSets(2 * VmcSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * VmcSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * VmcSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		initDescriptorsAndUBOs();

		initImgui();
		loadGameObjects();
		initSkeletons();
		initCollidables();
		viewerObject = std::make_unique<VmcGameObject>(VmcGameObject::createGameObject());
	}

	VmcApp::~VmcApp(){
		vkDestroyDescriptorPool(vmcDevice.device(), imGuiPool, nullptr);
	}

	void VmcApp::run()
	{

		simpleRenderSystem = std::make_unique<SimpleRenderSystem>(
			vmcDevice, 
			vmcRenderer.getSwapChainRenderPass(),
			vmcRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout());
	
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

			// FPS cap (minimumed to 2)
			while (frameTime < 1.0f / glm::max(animation_FPS, 2.0f))
			{
				auto newTime = std::chrono::high_resolution_clock::now();
				float diff = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
				currentTime = newTime;
				frameTime += diff;
			}

			// Controllers
			if(gameObjects[deformationIndex].deformationEnabled)
				ffdController.updateDeformationGrid(vmcWindow.getGLFWwindow(), frameTime, gameObjects[deformationIndex]);

			// Update rigid bodies
			for (auto& rigid : rigidBodies)
			{
				rigid.updateState(frameTime);
			}
			updateCamera(frameTime);
			checkRigidBodyCollisions();
			updateParticleSystems();
			storyboard.updateAnimatables(frameTime);
			skeletons[0].update();

			// Render loop
			if (auto commandBuffer = vmcRenderer.beginFrame()) {
				int frameIndex = vmcRenderer.getFrameIndex();
				
				// Update phase
				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				ubo.projection = camera.getProjection();;
				ubo.view = camera.getView();
				ubo.view[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				skyboxUbos[frameIndex]->writeToBuffer(&ubo);
				skyboxUbos[frameIndex]->flush();

				// Render phase
				vmcRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem->renderGameObjects(
					commandBuffer, 
					globalDescriptorSets[frameIndex], 
					skyboxDescriptorSets[frameIndex],
					skyboxObjects,
					gameObjects, 
					animators, 
					Lsystems, 
					skeletons[0], 
					rigidBodies, 
					collidables,
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

	void VmcApp::loadGameObject(const char* objName)
	{
		std::string objPath = std::string("../Models/") + std::string(objName);
		if (!std::filesystem::exists(objPath))
		{
			std::cout << "Model file does not exist!" << std::endl;
			return;
		}

		std::shared_ptr<VmcModel> model = VmcModel::createModelFromFile(vmcDevice, objPath.c_str());
		auto newObj = VmcGameObject::createGameObject();
		newObj.model = model;
		newObj.setPosition({ .0f, .0f, .0f });
		newObj.transform.rotation = { .0f, .0f, .0f };
		newObj.setScale({ 1.0f, 1.0f, 1.0f });
		gameObjects.push_back(std::move(newObj));
	}

 
	void VmcApp::loadGameObjects()
    {
		// Stick figure
		std::shared_ptr<VmcModel> stickModel = VmcModel::createModelFromFile(vmcDevice, "../Models/stick_fig/body_stick.obj");
		std::shared_ptr<VmcModel> armModel = VmcModel::createModelFromFile(vmcDevice, "../Models/stick_fig/arm_left.obj");

		auto stickObj = VmcGameObject::createGameObject();
		stickObj.model = stickModel;
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

		std::shared_ptr<VmcModel> skyboxModel = VmcModel::createModelFromFile(vmcDevice, "../Models/skybox.obj");
		auto skybox = VmcGameObject::createGameObject();
		skybox.model = skyboxModel;
		skybox.setPosition({ .0f, .0f, .0f });
		skybox.transform.rotation = { .0f, .0f, .0f };
		skybox.setScale({ 1.0f, 1.0f, 1.0f });
		skyboxObjects.push_back(std::move(skybox));


		std::shared_ptr<VmcModel> bolModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");
		auto sphere = VmcGameObject::createGameObject();
		sphere.model = bolModel;
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

	void VmcApp::loadTextures()
	{
		const char* test = "../Textures/pepe.jpg";
		testTexture = std::make_unique<VmcTexture>(vmcDevice, test, TEXTURE_TYPE_STANDARD_2D);

		const char* japanCubemapPath = "../Textures/cubemaps/cubemap_yokohama_rgba.ktx";
		skyboxTexture = std::make_unique<VmcTexture>(vmcDevice, japanCubemapPath, TEXTURE_TYPE_CUBE_MAP);
	}


	void VmcApp::initDescriptorsAndUBOs()
	{
		// Create UBOs
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

		skyboxUbos.resize(VmcSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < skyboxUbos.size(); i++)
		{
			skyboxUbos[i] = std::make_unique<VmcBuffer>(
				vmcDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			skyboxUbos[i]->map();
		}

		// Create set layouts
		 globalSetLayout = std::move(VmcDescriptorSetLayout::Builder(vmcDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)	// UBO
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)	// Texture sampler
			.build());

		// Write to descriptor sets
		globalDescriptorSets.resize(VmcSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto textureInfo = testTexture->descriptorInfo();

			// Write buffer info to binding 0, texture info to binding 1
			VmcDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImage(1, &textureInfo)
				.build(globalDescriptorSets[i]);
		}

		skyboxDescriptorSets.resize(VmcSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < skyboxDescriptorSets.size(); i++)
		{
			auto skyboxUboInfo = skyboxUbos[i]->descriptorInfo();
			auto skyboxTextureInfo = skyboxTexture->descriptorInfo();

			// Write buffer info to binding 0, texture info to binding 1
			VmcDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &skyboxUboInfo)
				.writeImage(1, &skyboxTextureInfo)
				.build(skyboxDescriptorSets[i]);
		}
	}


	void VmcApp::renderImGuiWindow()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Vulkan Animation Engine UI");
		ImGui::TextWrapped("General settings");

		ImGui::InputFloat("FPS cap ", &animation_FPS);
		ImGui::Checkbox("Skybox ", &simpleRenderSystem->shouldRenderSkybox());


		ImGui::Text("Camera Mode:");
		ImGui::RadioButton("Edit mode", &cameraMode, 0); ImGui::SameLine();
		ImGui::RadioButton("Free roam mode", &cameraMode, 1); ImGui::SameLine();
		ImGui::RadioButton("Animator mode", &cameraMode, 2);

		ImGui::Text("-----------------------------------------------------");
		{
			if (ImGui::Button("Path Animator", ImVec2(100, 25)))
			{
				UI_Tab = 0;
			}
			ImGui::SameLine();

			if (ImGui::Button("Deformation", ImVec2(100, 25)))
			{
				UI_Tab = 1;
			}

			ImGui::SameLine();

			if (ImGui::Button("Particles", ImVec2(100, 25)))
			{
				UI_Tab = 2;
			}

			if (ImGui::Button("L-Systems", ImVec2(100, 25)))
			{
				UI_Tab = 3;
			}

			ImGui::SameLine();

			if (ImGui::Button("Game Objects", ImVec2(100, 25)))
			{
				UI_Tab = 4;
			}

			ImGui::SameLine();

			if (ImGui::Button("Storyboard", ImVec2(100, 25)))
			{
				UI_Tab = 5;
			}

			if (ImGui::Button("Skeletons", ImVec2(100, 25)))
			{
				UI_Tab = 6;
			}
		}

		ImGui::Text("-----------------------------------------------------");
		
		switch (UI_Tab)
		{
		case 0:
			renderImGuiPathAnimatorUI();
			break;

		case 1:
			renderImGuiDeformationUI();
			break;

		case 2:
			renderImGuiParticleUI();
			break;

		case 3:
			renderImGuiLSystemUI();
			break;

		case 4:
			renderImGuiGameObjectsUI();
			break;

		case 5:
			renderImGuiStoryBoardUI();
			break;

		case 6:
			renderImGuiSkeletonUI();
			break;

		default:
			break;
		}

		ImGui::End();

		ImGui::Render();
	}

	void VmcApp::renderImGuiStoryBoardUI()
	{
		// =====================
		// STORYBOARD UI
		// =====================
		if (ImGui::Button("Start Animation"))
		{
			storyboard.startStoryBoardAnimation();
		}

		ImGui::Text("Storyboard panels:");
		int index = 0;
		for (auto a : storyboard.animatables)
		{
			ImGui::NewLine();
			std::string titleLabel = "Animatable ";
			ImGui::Text((titleLabel + std::to_string(index)).c_str());
			ImGui::SameLine();
			std::string removeAnimatableLabel = "Remove (";
			if (ImGui::Button((removeAnimatableLabel + std::to_string(index) + ")").c_str()))
			{
				storyboard.removeAnimatable(index);
			}

			std::string startTimeLabel = "Start time (";
			if (ImGui::InputFloat((startTimeLabel + std::to_string(index) + ")").c_str(), &a->getStartTime()))
			{
				storyboard.updateStoryBoardDuration();
			}
			std::string durationLabel = "Duration (";
			if (ImGui::InputFloat((durationLabel + std::to_string(index) + ")").c_str(), &a->getAnimationDuration()))
			{
				storyboard.updateStoryBoardDuration();
			}

			index++;
		}

		ImGui::NewLine();

		ImGui::Text("Available animatables:");
		index = 0;
		for (auto& a : animators)
		{
			if (!storyboard.containsAnimatable(&a))
			{
				std::string animatorButtonLabel = "Add Path Animator ";
				if (ImGui::Button((animatorButtonLabel + std::to_string(index)).c_str()))
				{
					storyboard.addAnimatable(&a);
				}
			}
			index++;;
		}

		index = 0;
		for (auto& d : gameObjects)
		{
			if (!d.deformationEnabled)
				continue;
			if (!storyboard.containsAnimatable(&d.deformationSystem))
			{
				std::string deformableButtonLabel = "Add deformable object ";
				if (ImGui::Button((deformableButtonLabel + std::to_string(index)).c_str()))
				{
					storyboard.addAnimatable(&d.deformationSystem);
				}
			}
			index++;
		}
	}

	// TODO: display error/success message after loading object
	void VmcApp::renderImGuiGameObjectsUI()
	{
		// =====================
		// GAMEOBJECTS UI
		// =====================
		ImGui::InputText(".obj file name", fileNameBuffer, 50 * sizeof(char));

		if (ImGui::Button("Add Game Object"))
		{
			loadGameObject(fileNameBuffer);
		}
		ImGui::NewLine();

		int index = 0;
		for (auto& obj : gameObjects) {
			std::string gameObjLabel = "GameObj ";
			ImGui::Text((gameObjLabel + std::to_string(index)).c_str());
			ImGui::SameLine();
			std::string removeGameObjLabel = "Delete (";
			if (ImGui::Button((removeGameObjLabel + std::to_string(index) + ")").c_str()))
			{
				gameObjects.erase(gameObjects.begin() + index);
			}

			std::string posLabel = "Pos (";
			if (ImGui::DragFloat3((posLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(obj.transform.translation)))
			{
				obj.setPosition(obj.transform.translation);
			};

			std::string rotLabel = "Rot (";
			if(ImGui::DragFloat3((rotLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(obj.transform.rotation), 0.01f, 0.0f, 2 * glm::pi<float>()))
			{
				obj.setRotation(obj.transform.rotation);
			}

			std::string scaleLabel = "Scale (";
			float uniformScaling = obj.transform.scale.x;
			if (ImGui::DragFloat((scaleLabel + std::to_string(index) + ")").c_str(), &uniformScaling, 0.01f, 0.01f, 5.0f))
			{
				obj.setScale(glm::vec3{uniformScaling, uniformScaling, uniformScaling});
			}

			std::string objColorLabel = "Color (";
			ImGui::ColorEdit3((objColorLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(obj.color));

			ImGui::NewLine();
			index++;
		}

	}


	void VmcApp::renderImGuiPathAnimatorUI()
	{
		// =====================
		// SPLINE ANIMATOR UI
		// =====================
		if (ImGui::Button("Add Spline Curve Animator"))
		{
			addSplineAnimator();
		}

		for (int i = 0; i < animators.size(); i++)
		{

			ImGui::TextWrapped("==============================================");
			std::string animatorTitle = "Spline animator ";
			ImGui::TextWrapped((animatorTitle + std::to_string(i)).c_str());

			ImGui::SameLine();
			std::string removeLabel = "Remove animator ";
			if (ImGui::Button((removeLabel + std::to_string(i)).c_str()))
			{
				animators.erase(animators.begin() + i);
			}

			// Select game object to animate over the path
			std::string objSelectTitle = "Animated object (";
			if (ImGui::BeginCombo((objSelectTitle + std::to_string(i) + ") ").c_str(), animators[i].currentObjSelected.c_str()))
			{
				for (int n = 0; n < gameObjects.size() + 2; n++)
				{
					if (n == 0)
					{
						bool is_selected = (animators[i].currentObjSelected == "None");
						if (ImGui::Selectable((std::string("Remove animated object for animator ") + std::to_string(i)).c_str(), is_selected))
						{
							animators[i].currentObjSelected = "None";
							animators[i].removeAnimatedObject();
						}
					}
					else if (n <= gameObjects.size()) {
						if (gameObjects[n - 1].onPathAnimator) continue;

						bool is_selected = (animators[i].currentObjSelected == std::to_string(gameObjects[n - 1].getId()));

						if (ImGui::Selectable((std::string("Add to animator ") + std::to_string(i) + ": Object " + std::to_string(gameObjects[n - 1].getId())).c_str(), is_selected))
						{
							animators[i].currentObjSelected = std::string("Object ") + std::to_string(gameObjects[n - 1].getId());
							// Remove the object that was currently being animated by the animator, and add the selected object
							animators[i].removeAnimatedObject();
							animators[i].addAnimatedObject(&gameObjects[n - 1]);
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					else {
						bool is_selected = (animators[i].currentObjSelected == "Camera");

						if (ImGui::Selectable((std::string("Add camera to animator ") + std::to_string(i)).c_str(), is_selected))
						{
							animators[i].currentObjSelected = "Camera";
							animators[i].removeAnimatedObject();
							animators[i].addAnimatedObject(viewerObject.get());
						}
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Text("Speed Control Function:"); ImGui::SameLine();
			ImGui::RadioButton("Sine", &animators[i].speedControl, 0); ImGui::SameLine();
			ImGui::RadioButton("Linear", &animators[i].speedControl, 1); ImGui::SameLine();
			ImGui::RadioButton("Parabolic", &animators[i].speedControl, 2); 

			std::string animatorMoveLabel = "Pos anim ";
			if (ImGui::DragFloat3((animatorMoveLabel + std::to_string(i)).c_str(), glm::value_ptr(animators[i].getPosition()), 1.0f, -20.0f, 20.0f))
			{
				animators[i].updateControlAndCurvePoints();
				animators[i].getSpline().generateSplineSegments();
				animators[i].buildForwardDifferencingTable();
			}

			ImGui::DragFloat3("Start orientation", glm::value_ptr(animators[i].getStartOrientation()), 1.0f, 0.0f, 2 * glm::pi<float>());
			ImGui::DragFloat3("End orientation", glm::value_ptr(animators[i].getEndOrientation()), 1.0f, 0.0f, 2 * glm::pi<float>());

			ImGui::NewLine();

			ImGui::Text("Spline control points: "); 
			ImGui::SameLine();
			std::string addCPLabel = "ADD CP (anim ";
			if (ImGui::Button((addCPLabel + std::to_string(i) + ")").c_str()))
			{
				animators[i].addControlPoint(ControlPoint{ {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.1f}, sphereModel }, animators[i].getPosition());
			}

			std::vector<VmcGameObject>& CPs = animators[i].getControlPoints();
			for (int j = 0; j < CPs.size(); j++)
			{
				std::string cpLabel = "CP ";
				if (ImGui::DragFloat3((cpLabel + std::to_string(i) + "." + std::to_string(j)).c_str(), glm::value_ptr(CPs[j].transform.translation), 1.0f, -50.0f, 50.0f)) {
					animators[i].getSpline().updateControlPointsRelativePositions();
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
	}

	void VmcApp::renderImGuiDeformationUI()
	{
		// =====================
		// DEFORMATION SYSTEM UI
		// =====================
		ImGui::Text("Currently deforming: ");
		for (int i = 0; i < gameObjects.size(); i++)
		{
			if (gameObjects[i].deformationEnabled)
			{
				ImGui::RadioButton(std::to_string(i).c_str(), &deformationIndex, i); ImGui::SameLine();
			}
		}
		ImGui::NewLine();
		ImGui::NewLine();

		int index = 0;
		for (auto& obj : gameObjects) {
			std::string gameObjLabel = "GameObj ";
			ImGui::Text((gameObjLabel + std::to_string(index)).c_str());

			// Deformation enabled checkbox
			std::string gameObjCheckLabel = "Deformation enabled ";
			if (ImGui::Checkbox((gameObjCheckLabel + "(" + std::to_string(index) + ")").c_str(), &obj.deformationEnabled))
			{
				if (obj.deformationEnabled)
				{
					obj.initDeformationSystem();
				}
				else {
					obj.resetObjectForm();
					obj.disableDeformationSystem();
				}
			}

			// Add keyframe button + list of keyframes
			if (obj.deformationEnabled)
			{
				std::string keyframeButtonLabel = "Add Keyframe to obj ";
				if (ImGui::Button((keyframeButtonLabel + std::to_string(index)).c_str()))
				{
					obj.deformationSystem.addKeyFrame();
					obj.confirmObjectDeformation();
					obj.resetObjectForm();
					obj.deformationSystem.resetControlPoints();
				}

				ImGui::Text("Keyframes: ");
				ImGui::NewLine();
				for (int j = 0; j < obj.deformationSystem.getAmountKeyframes(); j++)
				{
					std::string keyframeButtonLabel = "DEL ";
					if (ImGui::Button((keyframeButtonLabel + "(" + std::to_string(index) + "." + std::to_string(j) + ")").c_str()))
					{
						if (obj.deformationSystem.getAmountKeyframes())
						{
							obj.runAnimation = false;
						}
						obj.deformationSystem.delKeyFrame(j);
					}
				}
			}
			index++;
		}
	}
	

	void VmcApp::renderImGuiParticleUI()
	{
		// =====================
		// PARTICLE SYSTEM UI
		// =====================
		if (ImGui::Button("Add particle system"))
		{
			addParticleSystem();
		}

		if (particleSystems.size() > 0)
			ImGui::Text("Particle Systems:");
		ImGui::NewLine();

		int index = 0;
		for (auto& p : particleSystems)
		{
			std::string particleSysLabel = "Particle System (";
			ImGui::Text((particleSysLabel + std::to_string(index) + ")").c_str());

			ImGui::SameLine();

			std::string delLabel = "Delete (";
			if (ImGui::Button((delLabel + std::to_string(index) + ")").c_str()))
			{
				particleSystems.erase(particleSystems.begin() + index);
			}

			std::string activeLabel = "Active? (";
			ImGui::Checkbox((activeLabel + std::to_string(index) + ")").c_str(), &p.isOn);

			std::string particlePositionLabel = "Position (";
			ImGui::DragFloat3((particlePositionLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(p.position), 0.05f, -20.0f, 20.0f);
			
			std::string shootLabel = "Shoot direction (";
			ImGui::DragFloat3((shootLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(p.shootDirection), 0.01f, -1.0f, 1.0f);

			std::string powerLabel = "Power (";
			ImGui::InputFloat((powerLabel + std::to_string(index) + ")").c_str(), &p.power);

			std::string angleDevLabel = "Angle dev (";
			ImGui::InputFloat((angleDevLabel + std::to_string(index) + ")").c_str(), &p.angleDeviation);

			ImGui::NewLine();
			index++;
		}
	}

	void VmcApp::renderImGuiLSystemUI()
	{
		// =====================
		// L-SYSTEM UI
		// =====================
		if (ImGui::Button("Add Bush"))
		{
			addLSystem(BUSH);
		} ImGui::SameLine();
		if (ImGui::Button("Add long plant"))
		{
			addLSystem(LONG_PLANT);
		} ImGui::SameLine();
		if (ImGui::Button("Add tree"))
		{
			addLSystem(TREE);
		}

		if (Lsystems.size() > 0)
			ImGui::Text("L-Systems:");
		ImGui::NewLine();
		
		int index = 0;
		for (auto& l : Lsystems)
		{
			std::string LSysLabel = l.getVegetationType() + " L-System (";
			ImGui::Text((LSysLabel + std::to_string(index) + ")").c_str());

			std::string delLabel = "Delete (";
			if (ImGui::Button((delLabel + std::to_string(index) + ")").c_str()))
			{
				Lsystems.erase(Lsystems.begin() + index);
			}

			std::string LSysPositionLabel = "Position (";
			if (ImGui::DragFloat3((LSysPositionLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(l.rootPosition), 1.0f, -20.0f, 20.0f))
			{
				l.resetTurtleAndRerender();
			}
			std::string LSysColorLabel = "Color (";
			ImGui::ColorEdit3((LSysColorLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(l.renderColor));

			ImGui::NewLine();
			index++;
		}
	}

	void VmcApp::renderImGuiSkeletonUI()
	{
		ImGui::DragFloat3("Focus point pos", glm::value_ptr(skeletons[0].focusPoint),0.01f);

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

		SplineAnimator splineAnimator{ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, controlPoints, 4.0f, 0.0f };
		splineAnimator.getSpline().generateSplineSegments();
		animators.push_back(std::move(splineAnimator));

		// Build forward differencing table based on curve points
		animators.back().buildForwardDifferencingTable();
	}

	void VmcApp::addParticleSystem()
	{
		std::shared_ptr<VmcModel> waterDropModel = VmcModel::createModelFromFile(vmcDevice, "../Models/cube.obj");

		ParticleSystem hose{ {0.0f, 0.0f, 0.0f}, waterDropModel };
		particleSystems.push_back(hose);
	}

	void VmcApp::addLSystem(VegetationType type)
	{
		switch (type)
		{
		case vae::BUSH:
			Lsystems.push_back(LSystem{ "../Misc/l-systems/bush.txt", BUSH });
			Lsystems.back().mature();
			break;
		case vae::LONG_PLANT:
			Lsystems.push_back(LSystem{ "../Misc/l-systems/long_plant.txt", LONG_PLANT });
			Lsystems.back().mature();
			break;
		case vae::TREE:
			Lsystems.push_back(LSystem{ "../Misc/l-systems/tree.txt", TREE });
			Lsystems.back().mature();
			break;
		default:
			break;
		}
	}


	void VmcApp::initSkeletons()
	{
		std::shared_ptr<VmcModel> boneModel = VmcModel::createModelFromFile(vmcDevice, "../Models/bone.obj");
		
		Skeleton2 skeleton{boneModel};
		
		skeleton.addRoot({ 0.0f, 0.0f, 0.0f }, 1.0f, { 1.0f, 0.0f, 0.0f });
		skeleton.addBone(1.0f, { 90.0f, 90.0f, 0.0f });
		skeleton.addBone(1.0f, { 0.0f, 45.0f, 0.0f });
		skeleton.addBone(1.0f, { 0.0f, 45.0f, 0.0f });
		skeleton.addBone(1.0f, { 0.0f, 45.0f, 0.0f });
		skeleton.addBone(1.0f, { 0.0f, 45.0f, 0.0f });

		skeletons.push_back(skeleton);
	}

	void VmcApp::initCollidables()
	{
		std::vector<std::pair<glm::vec3, float>> massPoints1;
		massPoints1.push_back(std::make_pair(glm::vec3{ 1.0f, 0.0f, 0.0f }, 1.0f));

		std::shared_ptr<VmcModel> groundModel = VmcModel::createModelFromFile(vmcDevice, "../Models/ground.obj");
		RigidBody ground{ massPoints1, false, groundModel, {1.0f, 1.0f, 1.0f} };
		ground.S.pos = { 0.0f, 5.0f, 0.0f };
		collidables.push_back(ground);
	}


	void VmcApp::checkRigidBodyCollisions()
	{
		for (auto& rigid : rigidBodies)
		{
			for (auto& collidable : collidables)
			{
				CollisionInfo col{};
				rigid.detectCollision(collidable, col);
			}
		}
	}

	void VmcApp::updateParticleSystems()
	{
		for (auto& p : particleSystems)
		{
			p.generateParticles(rigidBodies);
		}
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