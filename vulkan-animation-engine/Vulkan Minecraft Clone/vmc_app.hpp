#pragma once
#include "vmc_renderer.hpp"
#include "vmc_device.hpp"
#include "vmc_window.hpp"
#include "vmc_game_object.hpp"
#include "animator.hpp"
#include "spline_animator.hpp"
#include "l_system.hpp"
#include "ffd.hpp"

// std 
#include <memory>
#include <vector>

namespace vmc {
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
	private:
		void loadGameObjects();
		void initLSystems();

		VmcWindow vmcWindow{ WIDTH, HEIGHT, "Vulkan Animation Engine - Jente Vandersanden" };
		VmcDevice vmcDevice{ vmcWindow };
		VmcRenderer vmcRenderer{ vmcWindow, vmcDevice };

		std::vector<LSystem> Lsystems;
		std::vector<VmcGameObject> gameObjects;
		std::vector<SplineAnimator> animators;
	};
}

