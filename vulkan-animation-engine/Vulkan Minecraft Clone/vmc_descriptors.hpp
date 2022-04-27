#pragma once

#include "vmc_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace vae {

    class VmcDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(VmcDevice& vmcDevice) : vmcDevice{ vmcDevice } {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);
            std::unique_ptr<VmcDescriptorSetLayout> build() const;

        private:
            VmcDevice& vmcDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        VmcDescriptorSetLayout(
            VmcDevice& vmcDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~VmcDescriptorSetLayout();
        VmcDescriptorSetLayout(const VmcDescriptorSetLayout&) = delete;
        VmcDescriptorSetLayout& operator=(const VmcDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        VmcDevice& vmcDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class VmcDescriptorWriter;
    };


    class VmcDescriptorPool {
    public:
        class Builder {
        public:
            Builder(VmcDevice& vmcDevice) : vmcDevice{ vmcDevice } {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<VmcDescriptorPool> build() const;

        private:
            VmcDevice& vmcDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        VmcDescriptorPool(
            VmcDevice& vmcDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~VmcDescriptorPool();
        VmcDescriptorPool(const VmcDescriptorPool&) = delete;
        VmcDescriptorPool& operator=(const VmcDescriptorPool&) = delete;

        bool allocateDescriptor(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();

    private:
        VmcDevice& vmcDevice;
        VkDescriptorPool descriptorPool;

        friend class VmcDescriptorWriter;
    };

    class VmcDescriptorWriter {
    public:
        VmcDescriptorWriter(VmcDescriptorSetLayout& setLayout, VmcDescriptorPool& pool);

        VmcDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        VmcDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        VmcDescriptorSetLayout& setLayout;
        VmcDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

}
