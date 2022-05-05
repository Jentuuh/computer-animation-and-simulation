#pragma once
#include <glm/glm.hpp>
namespace vae {

	struct BoundingBoxProperties
	{
		float minX;
		float maxX;
		float minY;
		float maxY;
		float minZ;
		float maxZ;
	};

	class BoundingBox
	{
	public:
		BoundingBox();
		BoundingBox(BoundingBoxProperties init);

		bool intersects(glm::vec3 pos, glm::vec3 pos_other, BoundingBox& other, glm::vec3& collisionNormal);
		BoundingBoxProperties props;

	};
}
