#pragma once
#include "vmc_renderer.hpp"
#include "vmc_device.hpp"
#include "vmc_window.hpp"
#include "vmc_game_object.hpp"
#include "vmc_descriptors.hpp"

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
		void initLSystems();
		void initSkeletons();
		void initRigidBodies();

		VmcWindow vmcWindow{ WIDTH, HEIGHT, "Vulkan Animation Engine - Jente Vandersanden" };
		VmcDevice vmcDevice{ vmcWindow };
		VmcRenderer vmcRenderer{ vmcWindow, vmcDevice };

		// Order of declarations matter!
		std::unique_ptr<VmcDescriptorPool> globalPool{};
		VkDescriptorPool imGuiPool;


		std::vector<LSystem> Lsystems;
		std::vector<VmcGameObject> gameObjects;
		std::vector<SplineAnimator> animators;
		std::vector<Skeleton> skeletons;
		std::vector<RigidBody> rigidBodies;
	};
}

