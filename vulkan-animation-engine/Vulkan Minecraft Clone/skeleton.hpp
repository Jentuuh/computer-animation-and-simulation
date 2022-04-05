#pragma once
#include "joint.hpp"

#include <memory>


namespace vae {
	class Skeleton
	{
	public:
		Skeleton();
		void initSampleSkeleton();
		void calculateTransformationMatrices();
		
	private:
		std::shared_ptr<Joint> root;
	};
}

