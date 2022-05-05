#pragma once
#include "vmc_renderer.hpp"
#include "vmc_device.hpp"
#include "vmc_window.hpp"
#include "vmc_game_object.hpp"
#include "vmc_descriptors.hpp"
#include "vmc_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "ffd_keyboard_controller.hpp"
#include "spline_keyboard_controller.hpp"

#include "animator.hpp"
#include "spline_animator.hpp"
#include "skeleton.hpp"
#include "l_system.hpp"
#include "ffd.hpp"
#include "rigid_body.hpp"

// std 
#include <memory>
#include <vector>

namespace vae {
	class VmcApp
	{
	public:
		const float MAX_FRAME_TIME = .1f;
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		VmcApp();
		~VmcApp();

		VmcApp(const VmcApp&) = delete;
		VmcApp& operator=(const VmcApp&) = delete;

		void run();
		void initImgui();
	private:
		void loadGameObjects();
		void initDescriptorsAndUBOs();

		void addSplineAnimator();
		void initLSystems();
		void initSkeletons();
		void initRigidBodies();
		void checkRigidBodyCollisions();

		void renderImGuiWindow();
		void renderImGuiPathAnimatorUI();
		void renderImGuiDeformationUI();

		void updateCamera(float frameTime);

		VmcWindow vmcWindow{ WIDTH, HEIGHT, "Vulkan Animation Engine - Jente Vandersanden" };
		VmcDevice vmcDevice{ vmcWindow };
		VmcRenderer vmcRenderer{ vmcWindow, vmcDevice };
		VmcCamera camera;
		std::unique_ptr<VmcGameObject> viewerObject{};

		// Order of declarations matter!
		std::unique_ptr<VmcDescriptorPool> globalPool{};
		VkDescriptorPool imGuiPool;	 // TODO: make use of VmcDescriptorPool class!

		std::vector<std::unique_ptr<VmcBuffer>> uboBuffers;
		std::unique_ptr<VmcDescriptorSetLayout> globalSetLayout;
		std::vector<VkDescriptorSet> globalDescriptorSets;

		std::vector<LSystem> Lsystems;
		std::vector<VmcGameObject> gameObjects;
		std::vector<SplineAnimator> animators;
		std::vector<Skeleton> skeletons;
		std::vector<RigidBody> rigidBodies;

		std::vector<RigidBody> collidables;

		KeyboardMovementController cameraController;
		SplineKeyboardController splineController;
		FFDKeyboardController ffdController;

		int cameraMode = 0;
		float animation_FPS = 60.0f;
		int UI_Tab = 0;
		int deformationIndex = 0;
		std::shared_ptr<VmcModel> sphereModel = VmcModel::createModelFromFile(vmcDevice, "../Models/sphere.obj");
	};
}

