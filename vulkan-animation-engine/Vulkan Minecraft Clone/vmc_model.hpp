#pragma once

#include "vmc_buffer.hpp"
#include "vmc_device.hpp"
#include "chunk_component.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE	// Forces that glm uses depth values in the [0;1] range, unlike the standard [-1;1] range.
#include <glm/glm.hpp>

// std 
#include <memory>
#include <vector>

namespace vae {
	class VmcModel
	{
	public:
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const {
				return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
			}
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
			float minX;
			float maxX;
			float minY;
			float maxY;
			float minZ;
			float maxZ;

			void loadModel(const std::string& filePath);
			void updateChunkMesh(const ChunkComponent* chunk);
		};

		VmcModel(VmcDevice &device, const VmcModel::Builder &builder);
		~VmcModel();

		VmcModel(const VmcModel&) = delete;
		VmcModel& operator=(const VmcModel&) = delete;

		float minimumX() { return minX; };
		float maximumX() { return maxX; };
		float minimumY() { return minY; };
		float maximumY() { return maxY; };
		float minimumZ() { return minZ; };
		float maximumZ() { return maxZ; };
		std::vector<Vertex>& getVertices() { return old_vertex_data; };

		static std::unique_ptr<VmcModel> createModelFromFile(VmcDevice& device, const std::string& filePath);
		static std::unique_ptr<VmcModel> createChunkModelMesh(VmcDevice& device, const ChunkComponent* chunk);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

		void updateVertices(std::vector<glm::vec3>& newPositions);
		void confirmModelDeformation();
		void updateVertexBuffers();

	private:
		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffers(const std::vector<uint32_t> &indices);

		float minX;
		float maxX;
		float minY;
		float maxY;
		float minZ;
		float maxZ;

		VmcDevice& vmcDevice;

		std::vector<Vertex> old_vertex_data;
		std::vector<Vertex> new_vertex_data;

		//VkBuffer vertexBuffer;
		//VkDeviceMemory vertexBufferMemory;
		std::unique_ptr<VmcBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<VmcBuffer> indexBuffer;
		//VkBuffer indexBuffer;
		//VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;
	};
}
