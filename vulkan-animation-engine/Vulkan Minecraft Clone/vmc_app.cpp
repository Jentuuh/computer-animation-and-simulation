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
			if (gameObjects.size() > deformationIndex)
			{
				if (gameObjects[deformationIndex].deformationEnabled)
					ffdController.updateDeformationGrid(vmcWindow.getGLFWwindow(), frameTime, gameObjects[deformationIndex]);
			}

			// Update rigid bodies
			for (auto& rigid : rigidBodies)
			{
				rigid.updateState(frameTime);
			}
			updateCamera(frameTime);
			checkRigidBodyCollisions();
			updateParticleSystems();
			storyboard.updateAnimatables(frameTime);

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
					skeletons, 
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

	void VmcApp::loadSceneFromFile(const char* fileName)
	{
		std::shared_ptr<VmcModel> sphereModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");
		std::shared_ptr<VmcModel> waterDropModel = VmcModel::createModelFromFile(vmcDevice, "../Models/cube.obj");
		std::string objPath = std::string("../Scenes/") + std::string(fileName);

		// Read from the text file
		std::ifstream readFile(objPath);
		std::string buffer;

		if (readFile.is_open())
		{
			// ========================
			// LOAD GAME OBJECTS
			// ========================
			if (std::getline(readFile, buffer)) {
				int numOfGameObjs = std::stoi(buffer);

				for (int i = 0; i < numOfGameObjs; i++)
				{
					// First line
					std::getline(readFile, buffer);
					char* lineString = &buffer[0];
					std::vector<char*> tokens = split(lineString, " ");
					int id = std::stoi(tokens[0]);
					char* objFileName = tokens[1];
					glm::vec3 pos = { std::stof(tokens[2]), std::stof(tokens[3]), std::stof(tokens[4]) };
					glm::vec3 rot = { std::stof(tokens[5]), std::stof(tokens[6]), std::stof(tokens[7]) };
					glm::vec3 scale = { std::stof(tokens[8]), std::stof(tokens[9]), std::stof(tokens[10]) };
					glm::vec3 color = { std::stof(tokens[11]), std::stof(tokens[12]), std::stof(tokens[13]) };
					bool deformationEnabled = std::stoi(tokens[14]);

					// Load in game object
					std::shared_ptr<VmcModel> model = VmcModel::createModelFromFile(vmcDevice, objFileName);
					auto newObj = VmcGameObject::createGameObject(id);	// Make sure that the id of the gameobjects is the same as in the saved file, otherwise animatables that refer to this ID might lose their reference
					newObj.modelPath = std::string(objFileName);
					newObj.model = model;
					newObj.deformationEnabled = deformationEnabled;
					if (newObj.deformationEnabled) newObj.initDeformationSystem();
					newObj.setPosition(pos);
					newObj.transform.rotation = rot;
					newObj.setScale(scale);
					newObj.color = color;
				
					// Amount keyframes line
					std::getline(readFile, buffer);
					int amountKeyframes = std::stoi(buffer);
					for (int j = 0; j < amountKeyframes; j++)
					{
						// Amount CPs line
						std::getline(readFile, buffer);
						int amountCPs = std::stoi(buffer);
						std::vector<glm::vec3> CPs;
						for (int k = 0; k < amountCPs; k++)
						{
							std::getline(readFile, buffer);
							lineString = &buffer[0];
							std::vector<char*> tokens = split(lineString, " ");
							glm::vec3 CPpos = { std::stof(tokens[0]), std::stof(tokens[1]), std::stof(tokens[2]) };
							CPs.push_back(CPpos);
						}
						newObj.deformationSystem.addKeyFrame(CPs);
					}
					gameObjects.push_back(std::move(newObj));
				}

				// ========================
				// LOAD ANIMATORS
				// ========================
				std::getline(readFile, buffer);
				int amountAnimators = std::stoi(buffer);
				for (int i = 0; i < amountAnimators; i++)
				{
					// First line
					std::getline(readFile, buffer);
					char* lineString = &buffer[0];
					std::vector<char*> tokens = split(lineString, " ");
					glm::vec3 pos = { std::stof(tokens[0]), std::stof(tokens[1]), std::stof(tokens[2]) };
					glm::vec3 startOr = { std::stof(tokens[3]), std::stof(tokens[4]), std::stof(tokens[5]) };
					glm::vec3 endOr = { std::stof(tokens[6]), std::stof(tokens[7]), std::stof(tokens[8]) };
					float startTime = std::stof(tokens[9]);
					float animationTime = std::stof(tokens[10]);

					// Amount CPs
					std::getline(readFile, buffer);
					int amountCPs = std::stoi(buffer);
					std::vector<ControlPoint> controlPoints{};
					for (int j = 0; j < amountCPs; j++)
					{
						std::getline(readFile, buffer);
						lineString = &buffer[0];
						std::vector<char*> tokens = split(lineString, " ");
						glm::vec3 CPpos = { std::stof(tokens[0]), std::stof(tokens[1]), std::stof(tokens[2]) };
						controlPoints.push_back({ CPpos, { 0.0f, 0.0f, 1.0f }, sphereModel });
					}

					SplineAnimator splineAnimator{ pos, startOr, endOr, controlPoints, animationTime, startTime };
					splineAnimator.getSpline().generateSplineSegments();
					animators.push_back(std::move(splineAnimator));

					// Build forward differencing table based on curve points
					animators.back().buildForwardDifferencingTable();

					std::getline(readFile, buffer);
					int amountAnimatedObjects = std::stoi(buffer);
					for (int j = 0; j < amountAnimatedObjects; j++)
					{
						std::getline(readFile, buffer);
						int animatedObjId = std::stoi(buffer);
						// TODO: add pointer to object with correct ID to the animator
					}
				}

				// ========================
				// LOAD PARTICLE SYSTEMS
				// ========================
				std::getline(readFile, buffer);
				int amountParticleSystems = std::stoi(buffer);
				for (int i = 0; i < amountParticleSystems; i++)
				{
					// First line
					std::getline(readFile, buffer);
					char* lineString = &buffer[0];
					std::vector<char*> tokens = split(lineString, " ");
					glm::vec3 pos = { std::stof(tokens[0]), std::stof(tokens[1]), std::stof(tokens[2]) };

					ParticleSystem newParticleSystem { pos, waterDropModel };

					// Amount keyframes line
					std::getline(readFile, buffer);
					int amountKeyframes = std::stoi(buffer);
					std::vector<ParticleKeyFrame> keyframes;
					for (int j = 0; j < amountKeyframes; j++)
					{
						// First line
						std::getline(readFile, buffer);
						char* lineString = &buffer[0];
						std::vector<char*> tokens = split(lineString, " ");
						glm::vec3 posKf = { std::stof(tokens[0]), std::stof(tokens[1]), std::stof(tokens[2]) };
						glm::vec3 shootDirKf = { std::stof(tokens[3]), std::stof(tokens[4]), std::stof(tokens[5]) };
						float power = std::stof(tokens[6]);
						keyframes.push_back({ posKf, shootDirKf, power });
					}
					newParticleSystem.addKeyFrames(keyframes);
					particleSystems.push_back(newParticleSystem);
				}

				// ========================
				// LOAD L-SYSTEMS
				// ========================
				std::getline(readFile, buffer);
				int amountLsystems = std::stoi(buffer);
				for (int i = 0; i < amountLsystems; i++)
				{
					// First line
					std::getline(readFile, buffer);
					char* lineString = &buffer[0];
					std::vector<char*> tokens = split(lineString, " ");
					std::cout << lineString << std::endl;
					int veg_type = std::stoi(tokens[0]);
					glm::vec3 pos = { std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]) };
					glm::vec3 col = { std::stof(tokens[4]), std::stof(tokens[5]), std::stof(tokens[6]) };

					addLSystem(static_cast<VegetationType>(veg_type));
					Lsystems.back().rootPosition = pos;
					Lsystems.back().renderColor = col;
					Lsystems.back().mature();
					Lsystems.back().resetTurtleAndRerender();
				}
			}
			// Close the file
			readFile.close();
		}
		else std::cout << "Unable to open file '" << fileName << "'.";
	}


	void VmcApp::saveSceneToFile(const char* fileName)
	{
		std::string objPath = std::string("../Scenes/") + std::string(fileName);

		std::ofstream saveFile(objPath);
		if (saveFile.is_open())
		{
			// GAME OBJECT FILE FORMAT: <id> <objfilename> <posX> <posY> <posZ> <rotX> <rotY> <rotZ> <scaleX> <scaleY> <scaleZ> <deformationEnabled> \n 
			//					<amountKeyFrames> \n
			//					for each keyframe:
			//						<amountCPs> \n
			//						for each CP:
			//							<posCPX> <posCPY> <posCPZ> 
		
			// Amount gameobjects
			saveFile << gameObjects.size() << std::endl;
			for (auto& g : gameObjects)
			{
				// First line
				saveFile << g.getId() << " " << g.modelPath << " " << g.transform.translation.x << " " << g.transform.translation.y << " "
					<< g.transform.translation.z << " " << g.transform.rotation.x << " " << g.transform.rotation.y << " "
					<< g.transform.rotation.z << " " << g.transform.scale.x << " " << g.transform.scale.y << " "
					<< g.transform.scale.z << " " << g.color.x << " " << g.color.y << " " << g.color.z << " " << g.deformationEnabled << std::endl;
				
				// Amount keyframes
				saveFile << g.deformationSystem.getAmountKeyframes() << std::endl;
				for (auto& k : g.deformationSystem.getKeyFrames())
				{
					// Amount control points 
					saveFile << k.size() << std::endl;
					for (auto& cp : k)
					{
						saveFile << cp.x << " " << cp.y << " " << cp.z << std::endl;
					}
				}
				
			}

			// ANIMATOR FILE FORMAT: <posX> <posY> <posZ> <startOrX> <startOrY> <startOrZ> <endOrX> <endOrY> <endOrZ> <startTime> <animationTime> \n
			//						<amountCPs> \n
			//						for each CP:
			//							<posCPX> <posCPY> <posCPZ> \n
			saveFile << animators.size() << std::endl;
			for (auto& a : animators)
			{
				saveFile << a.getPosition().x << " " << a.getPosition().y << " " << a.getPosition().z << " " << a.getStartOrientation().x << " "
					<< a.getStartOrientation().y << " " << a.getStartOrientation().z << " " << a.getEndOrientation().x << " "
					<< a.getEndOrientation().y << " " << a.getEndOrientation().z << " " << a.getStartTime() << " " << a.getAnimationDuration() << std::endl;

				// Amount CPs
				saveFile << a.getControlPoints().size() << std::endl;
				for (auto& cp : a.getControlPoints())
				{
					saveFile << cp.transform.translation.x << " " << cp.transform.translation.y << " " << cp.transform.translation.z << std::endl;
				}

				// Animated objects
				// Amount animated objects
				saveFile << a.getAmountAnimatedObjects() << std::endl;
				for (auto id : a.getAnimatedObjectIds())
				{
					saveFile << id << std::endl;
				}
			}

			// PARTICLE SYSTEM FILE FORMAT: <posX> <posY> <posZ> \n
			//						<amountKeyFrames> \n
			//						For each keyframe:
			//							<posX> <posY> <posZ> <shootDirX> <shootDirY> <shootDirZ> <power> \n

			// Amount particle systems
			saveFile << particleSystems.size() << std::endl;
			for (auto& p : particleSystems)
			{
				saveFile << p.position.x << " " << p.position.y << " " << p.position.z << std::endl;

				saveFile << p.getAmountKeyFrames() << std::endl;				
				for (auto& kf : p.getKeyFrames())
				{
					saveFile << kf.pos.x << " " << kf.pos.y << " " << kf.pos.z << " " << kf.shootDir.x << " " << kf.shootDir.y << " " << kf.shootDir.z << " " << kf.power << std::endl;
				}
			}

			// L SYSTEM FILE FORMAT: <fileName> <posX> <posY> <posZ> <colX> <colY> <colZ> \n

			// Amount L-systems
			saveFile << Lsystems.size() << std::endl;
			for (auto& l : Lsystems)
			{
				saveFile << l.getVegetationTypeEnum() << " " << l.rootPosition.x << " " << l.rootPosition.y << " " << l.rootPosition.z << " " << l.renderColor.x << " " << l.renderColor.y << " " << l.renderColor.z << std::endl;
			}

			saveFile.close();
		}
		else std::cout << "Unable to open file '" << fileName << "'.";
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
		newObj.modelPath = objPath;
		newObj.model = model;
		newObj.setPosition({ .0f, .0f, .0f });
		newObj.transform.rotation = { glm::pi<float>(), .0f, .0f };
		newObj.setScale({ 1.0f, 1.0f, 1.0f });
		gameObjects.push_back(std::move(newObj));
	}

	void VmcApp::loadSkeleton(const char* fileName)
	{
		std::string objPath = std::string("../Misc/") + std::string(fileName);
		std::shared_ptr<VmcModel> boneModel = VmcModel::createModelFromFile(vmcDevice, "../Models/bone.obj");

		Skeleton2 skeleton{ boneModel };

		// Read from the text file
		std::ifstream readFile(objPath);
		std::string buffer;

		if (readFile.is_open())
		{
			// Root
			std::getline(readFile, buffer);
			char* lineString = &buffer[0];
			std::vector<char*> tokens = split(lineString, " ");

			glm::vec3 pos = { std::stof(tokens[0]), std::stof(tokens[1]), std::stof(tokens[2]) };
			float len = std::stof(tokens[3]);
			glm::vec3 rot = { std::stof(tokens[4]), std::stof(tokens[5]), std::stof(tokens[6]) };
			skeleton.addRoot(pos, len, rot);

			// Number of bones
			std::getline(readFile, buffer);
			int numOfBones = std::stoi(buffer);
			for (int i = 0; i < numOfBones; i++)
			{
				// Bone
				std::getline(readFile, buffer);
				char* lineString = &buffer[0];
				std::vector<char*> tokens = split(lineString, " ");

				float len = std::stof(tokens[0]);
				glm::vec3 rot = { std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]) };
				skeleton.addBone(len, rot);
			}
			skeletons.push_back(skeleton);
			// Close the file
			readFile.close();
		}

		else std::cout << "Unable to open file '" << fileName << "'.";
	}


 
	void VmcApp::loadGameObjects()
    {
		// Skybox model
		std::shared_ptr<VmcModel> skyboxModel = VmcModel::createModelFromFile(vmcDevice, "../Models/skybox.obj");
		auto skybox = VmcGameObject::createGameObject();
		skybox.modelPath = std::string("../Models/skybox.obj");
		skybox.model = skyboxModel;
		skybox.setPosition({ .0f, .0f, .0f });
		skybox.transform.rotation = { .0f, .0f, .0f };
		skybox.setScale({ 1.0f, 1.0f, 1.0f });
		skyboxObjects.push_back(std::move(skybox));
	}

	void VmcApp::loadTextures()
	{
		const char* test = "../Textures/pepe.jpg";
		testTexture = std::make_unique<VmcTexture>(vmcDevice, test, TEXTURE_TYPE_STANDARD_2D);

		const char* japanCubemapPath = "../Textures/cubemaps/cubemap_vulkan.ktx";
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
		ImGui::RadioButton("Animated mode", &cameraMode, 2);

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

			if (ImGui::Button("Kinematics", ImVec2(100, 25)))
			{
				UI_Tab = 6;
			}
			ImGui::SameLine();

			if (ImGui::Button("Save/Load scene", ImVec2(100, 25)))
			{
				UI_Tab = 7;
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

		case 7:
			renderImGuiSaveLoadUI();
			break;


		default:
			break;
		}

		ImGui::End();

		ImGui::Render();
	}

	void VmcApp::renderImGuiSaveLoadUI()
	{
		ImGui::InputText(".vaescene scene file name", saveLoadFileName, 50 * sizeof(char));

		if (ImGui::Button("Load scene file"))
		{
			loadSceneFromFile(saveLoadFileName);
		}

		if (ImGui::Button("Save scene to file"))
		{
			saveSceneToFile(saveLoadFileName);
		}
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

		index = 0;
		for (auto& p : particleSystems)
		{
			if (!storyboard.containsAnimatable(&p))
			{
				std::string pSystemLabel = "Add particle system ";
				if (ImGui::Button((pSystemLabel + std::to_string(index)).c_str()))
				{
					storyboard.addAnimatable(&p);
				}
			}
			index++;
		}

		index = 0;
		for (auto& s : skeletons)
		{
			if (!storyboard.containsAnimatable(&s))
			{
				std::string skeletonLabel = "Add skeleton ";
				if (ImGui::Button((skeletonLabel + std::to_string(index)).c_str()))
				{
					storyboard.addAnimatable(&s);
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
			if (ImGui::DragFloat3((posLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(obj.transform.translation), 0.01f))
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
						if (ImGui::Selectable((std::string("Set no animated object for animator ") + std::to_string(i)).c_str(), is_selected))
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

			std::string startOrLabel = "Start orientation anim ";
			std::string endOrLabel = "End orientation anim ";

			ImGui::DragFloat3((startOrLabel + std::to_string(i)).c_str(), glm::value_ptr(animators[i].getStartOrientation()), 0.01f, 0.0f, 2 * glm::pi<float>());
			ImGui::DragFloat3((endOrLabel + std::to_string(i)).c_str(), glm::value_ptr(animators[i].getEndOrientation()), 0.01f, 0.0f, 2 * glm::pi<float>());

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

			// Keyframes
			std::string addLabel = "Add keyframe (";
			if (ImGui::Button((addLabel + std::to_string(index) + ")").c_str()))
			{
				p.addKeyFrame();
			}
			for (int i = 0; i < p.getAmountKeyFrames(); i++)
			{
				std::string kfLabel = "Keyframe (";
				ImGui::Text((kfLabel + std::to_string(i) + ")").c_str());
				ImGui::SameLine();
				std::string delLabel = "Delete (";
				if (ImGui::Button((delLabel + std::to_string(index) + ")").c_str()))
				{
					p.deleteKeyFrame(i);
				}
			}
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
		ImGui::InputText(".skel file name", skeletonFileName, 50 * sizeof(char));

		if (ImGui::Button("Add skeleton"))
		{
			loadSkeleton(skeletonFileName);
		}
		ImGui::NewLine();


		int index = 0;
		for (auto& l : skeletons)
		{
			std::string SkeletonLabel = "Skeleton (";
			ImGui::Text((SkeletonLabel + std::to_string(index) + ")").c_str());

			ImGui::SameLine();

			std::string delLabel = "Delete (";
			if (ImGui::Button((delLabel + std::to_string(index) + ")").c_str()))
			{
				skeletons.erase(skeletons.begin() + index);
			}

			std::string modeLabel = "Kinematics mode (";
			ImGui::Text((modeLabel + std::to_string(index) + ")").c_str());
			ImGui::RadioButton("Forward", &skeletons[index].mode, 0); ImGui::SameLine();
			ImGui::RadioButton("Inverse", &skeletons[index].mode, 1);

			std::string addKFLabel = "Add keyframe (";
			if (ImGui::Button((addKFLabel + std::to_string(index) + ")").c_str()))
			{
				if (skeletons[index].mode == FORWARD)
				{
					skeletons[index].addKeyFrameFK();
				}
				else if (skeletons[index].mode == INVERSE){
					skeletons[index].addKeyFrameIK();
				}
			}
			ImGui::NewLine();

			if (skeletons[index].mode == INVERSE)
			{
				std::string IKLabel = "IK (";
				ImGui::Text((IKLabel + std::to_string(index) + ")").c_str());

				std::string targetLabel = "Target pos (";
				if (ImGui::DragFloat3((targetLabel + std::to_string(index) + ")").c_str(), glm::value_ptr(skeletons[index].focusPoint), 0.1f))
				{
					skeletons[index].solveIK_Z();
				}
				std::string drawLabel = "Draw target (";
				ImGui::Checkbox((drawLabel + std::to_string(index) + ")").c_str(), &skeletons[index].drawIKTarget);
			}
			else if (skeletons[index].mode == FORWARD) {

				ImGui::Text("Skeleton bones:");
				std::vector<std::shared_ptr<Bone>> bones = skeletons[index].getBones();
				for (int i = 0; i < bones.size(); i++)
				{
					std::string rotLabel = "Rotation (";
					if (ImGui::DragFloat3((rotLabel + std::to_string(i) + ")").c_str(), glm::value_ptr(bones[i]->getRotation()), 1.0f))
					{
						bones[i]->updateRotation();
					}
				}
			}
			ImGui::NewLine();
			index++;
		}
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
		skeleton.addBone(3.0f, { 90.0f, 90.0f, 0.0f });
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

	std::vector<char*> VmcApp::split(char* stringToSplit, const char* separator)
	{
		std::vector<char*> result;

		char* next_token = NULL;
		char* token = strtok_s(stringToSplit, separator, &next_token);

		while ((token != NULL))
		{
			result.push_back(token);
			token = strtok_s(NULL, separator, &next_token);
		}
		return result;
	}

}