#pragma once
#include "vmc_model.hpp"
#include "chunk_component.hpp"
#include "ffd.hpp"

// libs
#include <glm/gtc/matrix_transform.hpp>

// std 
#include <memory>

namespace vae {

 // TODO: Make `Component` a base type?
 // TODO: Find cleaner solution for chunk management! (each gameobject currently contains a chunk)

    struct TransformComponent {
        glm::vec3 translation{};  // Position offset
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::vec3 rotation{};
        glm::vec3 relativePos{.0f, .0f, .0f};

        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };
     
    class VmcGameObject {

    public:
        using id_t = unsigned int;

        static VmcGameObject createGameObject() {
            static id_t currentId = 0;


            return VmcGameObject{ currentId++ };
        }
        ~VmcGameObject() 
        {
            static id_t currentId = currentId--;
        }

        VmcGameObject(const VmcGameObject&) = delete;
        VmcGameObject& operator=(const VmcGameObject&) = delete;
        VmcGameObject(VmcGameObject&&) = default;
        VmcGameObject& operator=(VmcGameObject&&) = default;

        id_t getId() { return id; }
        std::vector<VmcGameObject>& getChildren() { return children; };

        void addChild(VmcGameObject* child);
        void setPosition(glm::vec3 newPosition);
        void setRotation(glm::vec3 newRotation);
        void setScale(glm::vec3 newScale);

        void deformObject();
        void resetObjectForm();
        void setInitialAnimationForm();
        void confirmObjectDeformation();
        void initDeformationSystem();
        void disableDeformationSystem();

        std::shared_ptr<VmcModel> model{};
        std::vector<VmcGameObject> children{};
        glm::vec3 color{};
        TransformComponent transform{};

        FFD deformationSystem;
        bool deformationEnabled = false;
        bool runAnimation = false;

    private:
        VmcGameObject(id_t objId) : id{ objId } {}
        id_t id;
    };
}

