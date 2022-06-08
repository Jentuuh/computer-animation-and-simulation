#pragma once
#include "vmc_renderer.hpp"
#include "vmc_device.hpp"
#include "vmc_window.hpp"
#include "vmc_game_object.hpp"
#include "vmc_descriptors.hpp"
#include "vmc_camera.hpp"
#include "vmc_texture.hpp"
#include "keyboard_movement_controller.hpp"
#include "ffd_keyboard_controller.hpp"
#include "particle_system.hpp"
#include "simple_render_system.hpp"
#include "story_board.hpp"

#include "animator.hpp"
#include "spline_animator.hpp"
#include "skeleton2.hpp"
#include "l_system.hpp"
#include "ffd.hpp"
#include "rigid_body.hpp"

// std 
#include <memory>
#include <vector>
#include <fstream>

namespace vae {
	class VmcApp
	{
	public:
		const float MAX_FRAME_TIME = .1f;
		static constexpr int WIDTH = 1000;
		static constexpr int HEIGHT = 700;

		VmcApp();
		~VmcApp();

		VmcApp(const VmcApp&) = delete;
		VmcApp& operator=(const VmcApp&) = delete;

		void run();
		void initImgui();

	private:
		void loadSceneFromFile(const char* fileName);
		void saveSceneToFile(const char* fileName);
		void loadGameObject(const char * objName);
		void loadGameObjects();
		void loadTextures();
		void initDescriptorsAndUBOs();

		void addSplineAnimator();
		void addParticleSystem();
		void addLSystem(VegetationType type);

		void initSkeletons();
		void initCollidables();

		void checkRigidBodyCollisions();
		void updateParticleSystems();

		void renderImGuiWindow();
		void renderImGuiSaveLoadUI();
		void renderImGuiStoryBoardUI();
		void renderImGuiGameObjectsUI();
		void renderImGuiPathAnimatorUI();
		void renderImGuiDeformationUI();
		void renderImGuiParticleUI();
		void renderImGuiLSystemUI();
		void renderImGuiSkeletonUI();

		void updateCamera(float frameTime);

		std::vector<char*> split(char* stringToSplit, const char* separator);

		VmcWindow vmcWindow{ WIDTH, HEIGHT, "Vulkan Animation Engine - Jente Vandersanden" };
		VmcDevice vmcDevice{ vmcWindow };
		VmcRenderer vmcRenderer{ vmcWindow, vmcDevice };
		VmcCamera camera;
		std::unique_ptr<SimpleRenderSystem> simpleRenderSystem;
		std::unique_ptr<VmcGameObject> viewerObject{};

		// Order of declarations matter!
		std::unique_ptr<VmcDescriptorPool> globalPool{};
		VkDescriptorPool imGuiPool;	 // TODO: make use of VmcDescriptorPool class!

		std::unique_ptr<VmcTexture> testTexture;
		std::vector<std::unique_ptr<VmcBuffer>> uboBuffers;
		std::unique_ptr<VmcDescriptorSetLayout> globalSetLayout;
		std::vector<VkDescriptorSet> globalDescriptorSets;

		std::unique_ptr<VmcTexture> skyboxTexture;
		std::vector<std::unique_ptr<VmcBuffer>> skyboxUbos;
		std::vector<VkDescriptorSet> skyboxDescriptorSets;
		std::vector<VmcGameObject> skyboxObjects;

		std::vector<VmcGameObject> gameObjects;
		std::vector<SplineAnimator> animators;
		std::vector<Skeleton2> skeletons;
		std::vector<RigidBody> rigidBodies;
		std::vector<ParticleSystem> particleSystems;
		std::vector<LSystem> Lsystems;

		std::vector<RigidBody> collidables;

		StoryBoard storyboard;

		KeyboardMovementController cameraController;
		FFDKeyboardController ffdController;

		int cameraMode = 0;
		float animation_FPS = 120.0f;
		int UI_Tab = 0;
		int deformationIndex = 0;
		char fileNameBuffer[50] = "Your file name";
		char saveLoadFileName[50] = "Your file name";
		std::shared_ptr<VmcModel> sphereModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");
	};
}

