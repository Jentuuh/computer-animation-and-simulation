#include "bounding_box.hpp"

namespace vae {
	BoundingBox::BoundingBox(){
		props.minX = 0.0f;
		props.maxX = 0.0f;
		props.minY = 0.0f;
		props.maxY = 0.0f;
		props.minZ = 0.0f;
		props.maxZ = 0.0f;
	}

	BoundingBox::BoundingBox(BoundingBoxProperties init): props{init}{}

	// (Axis aligned bounding box algorithm)
	bool BoundingBox::intersects(glm::vec3 pos, glm::vec3 pos_other, BoundingBox& other, glm::vec3& collisionNormal)
	{
	
		if (((pos_other.x + other.props.minX <= pos.x + props.minX && pos.x + props.minX <= pos_other.x + other.props.maxX) ||	// X-collision
			(pos_other.x + other.props.minX <= pos.x + props.maxX && pos.x + props.maxX <= pos_other.x + other.props.maxX)) &&
			((pos_other.y + other.props.minY <= pos.y + props.minY && pos.y + props.minY <= pos_other.y + other.props.maxY) ||  // Y-collision
			(pos_other.y + other.props.minY <= pos.y + props.maxY && pos.y + props.maxY <= pos_other.y + other.props.maxY)) &&
			((pos_other.z + other.props.minZ <= pos.z + props.minZ && pos.z + props.minZ <= pos_other.z + other.props.maxZ) ||  // Z-collision
			(pos_other.z + other.props.minZ <= pos.z + props.maxZ && pos.z + props.maxZ <= pos_other.z + other.props.maxZ)))
		{
			collisionNormal = { 0.0f, -1.0f, 0.0f };
			return true;
		}
		return false;
	}
}