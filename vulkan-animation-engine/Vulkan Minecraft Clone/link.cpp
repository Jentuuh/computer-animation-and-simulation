#include "link.hpp"
#include "joint.hpp"

namespace vae {

	Link::Link(float a, float alpha): a{ a }, alpha{ alpha }
	{
		nextJoint = nullptr;
	}

	void Link::setNextJoint(std::shared_ptr<Joint> next)
	{
		nextJoint = next;
	}

}