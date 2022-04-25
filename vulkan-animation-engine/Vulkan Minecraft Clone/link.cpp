#include "link.hpp"
#include "joint.hpp"

namespace vae {

	Link::Link(float a, float alpha, std::shared_ptr<VmcModel> model): a{ a }, alpha{ alpha }, linkModel{ model }
	{
		nextJoint = nullptr;
	}

	void Link::setNextJoint(std::shared_ptr<Joint> next)
	{
		nextJoint = next;
	}

	void Link::setPrevJoint(std::shared_ptr<Joint> prev)
	{
		prevJoint = prev;
	}

}