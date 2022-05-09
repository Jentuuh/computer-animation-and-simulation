#pragma once
#include "vmc_device.hpp"
#include <ktx.h>
#include <ktxvulkan.h>

namespace vae {

	enum VmcTextureType {
		TEXTURE_TYPE_STANDARD_2D,
		TEXTURE_TYPE_CUBE_MAP,
	};

	class VmcTexture
	{
	public:
		VmcTexture(VmcDevice& device, const char* imagePath, VmcTextureType textureType);
		~VmcTexture();

		void createTextureImage(const char* imagePath);
		void createTextureImageCubeMap(const char* imagePath);

		void setupCubeMap(const char* imagePath, VkFormat format);
		void createTextureImageView();
		void createTextureImageViewCubeMap();
		void createTextureSampler(VkSamplerAddressMode addressMode);
		VkDescriptorImageInfo descriptorInfo();

	private:
		VmcDevice& device;

		uint32_t width;
		uint32_t height;
		uint32_t mipLevels = 1;

		VkImage textureImage;
		VkImageView textureImageView;
		VkDeviceMemory textureImageMemory;
		VkSampler textureSampler;
	};
}
