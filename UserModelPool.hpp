//
// Created by Carlo Ronconi on 27/06/23.
//

#include <utility>

#include "DataStructs.hpp"

#ifndef DRONE_DELIVERY_USERMODELPOOL_HPP
#define DRONE_DELIVERY_USERMODELPOOL_HPP

template <typename VertexType, typename UboType>
class UserModel {
private:
    Model<VertexType> model;
    DescriptorSet ds;
    Texture* texture;
    ModelType MOD_TYPE;
    std::string fileName;
    int currImage;

public:
    UboType ubo;

    UserModel(Texture *texture, std::string fileName, ModelType MOD_TYPE) :
              texture(texture),
              fileName(std::move(fileName)),
              MOD_TYPE(MOD_TYPE){}

    void initModel(BaseProject *bp, VertexDescriptor *vDescriptor) {
        model.init(bp, vDescriptor, fileName, MOD_TYPE);
    }
    void initDS(BaseProject *bp, DescriptorSetLayout *DSL) {
        ds.init(bp, DSL, {
                {0, UNIFORM, sizeof(UboType), nullptr},
                {1, TEXTURE, 0, texture}});
    }
    void cleanupDS() {
        ds.cleanup();
    }
    void cleanupModel() {
        model.cleanup();
    }
    void bind(VkCommandBuffer *commandBuffer, int currentImage, Pipeline *pipeline) {
        model.bind(*commandBuffer);
        ds.bind(*commandBuffer, *pipeline, 1, currentImage);
        vkCmdDrawIndexed(*commandBuffer, static_cast<uint32_t>(model.indices.size()),
                         1, 0, 0, 0);
        currImage = currentImage;
    }
    void map(glm::mat4 *world, glm::mat4 *view, glm::mat4 *projection) {
        ubo.amb = 1.0f; ubo.gamma = 180.0f; ubo.sColor = glm::vec3(1.0f);
        ubo.mvpMat = *projection * *view * *world;
        ubo.mMat = *world;
        ubo.nMat = glm::inverse(glm::transpose(*world));
        ds.map(currImage, &ubo, sizeof(ubo), 0);
    }
};

template <typename VertexType, typename UboType>
class UserModelPool {
private:
    VertexDescriptor* vDescriptor{};
    Pipeline* pipeline{};
    DescriptorSetLayout* DSL{};
    BaseProject* bp{};
public:
    std::vector<UserModel<VertexType, UboType>> models;

    UserModelPool() = default;
    UserModelPool(VertexDescriptor *vDescriptor, Pipeline *pipeline,
                  DescriptorSetLayout *DSL, BaseProject *bp) :
            vDescriptor(vDescriptor),
            pipeline(pipeline),
            DSL(DSL),
            bp(bp){}

    void initAllModels() {
        for (auto &m : models) {
            m.initModel(bp, vDescriptor);
        }
    }
    void initAllDSs() {
        for (auto &m : models) {
            m.initDS(bp, DSL);
        }
    }
    void cleanupAllDSs() {
        for (auto &m : models) {
            m.cleanupDS();
        }
    }
    void cleanupAllModels() {
        for (auto &m : models) {
            m.cleanupModel();
        }
    }
    void bindAll(VkCommandBuffer *commandBuffer, int currentImage) {
        for (auto &m : models) {
            m.bind(commandBuffer, currentImage, pipeline);
        }
    }
};

#endif //DRONE_DELIVERY_USERMODELPOOL_HPP
