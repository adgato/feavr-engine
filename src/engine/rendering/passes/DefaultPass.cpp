#include "DefaultPass.h"

#include <json.hpp>
#include "vk_engine.h"
#include "vk_pipelines.h"
#include "../shader_descriptors.h"
#include "assets-system/AssetFile.h"
#include "assets-system/AssetManager.h"

namespace rendering::passes
{
    namespace shader = shader_layouts::default_shader;
    
    void PassInst<DefaultPass>::Init(const std::shared_ptr<DefaultPass>& defaultPass)
    {
        VulkanEngine* engine = defaultPass->engine;

        matConstProperty = Buffer<default_pass::Instance::MaterialConstants>::Allocate(engine->vmaAllocator, 1,
                                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        matConstProperty.Access([](default_pass::Instance::MaterialConstants* data)
        {
            data->colorFactors = glm::vec4{1, 1, 1, 1};
            data->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};
        });
        
        properties.AllocateSet(engine->descriptorAllocator, defaultPass->materialLayout);
        properties.StageBuffer(default_pass::GLTFMaterialData_binding, matConstProperty);
        properties.StageImage(default_pass::colorTex_binding, engine->commonTextures.errorCheckerboard);
        properties.StageSampler(default_pass::colorTexSampler_binding, engine->defaultSamplerNearest);
        properties.PerformWrites();
        
        PassInstance::Init(defaultPass);
    }

    void PassInst<DefaultPass>::Destroy()
    {
        matConstProperty.Destroy();
    }

    void DefaultPass::ConfigurePass(const VkCommandBuffer cmd)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &engine->gpuSceneDescriptorSet.descriptorSet, 0, nullptr);
    }

    void PassInst<DefaultPass>::ConfigureInstance(const VkCommandBuffer cmd)
    {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->layout, 1, 1, &properties.descriptorSet, 0, nullptr);
    }

    void DefaultPass::DrawMesh(const VkCommandBuffer cmd, const SubMesh& submesh)
    {
        vkCmdBindIndexBuffer(cmd, submesh.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        for (ecs::Entity entity : submesh.entities)
        {
            const PushConstants pushConstants{manager->GetComponent<Transform>(entity).transform, submesh.mesh->vertexBufferAddress};

            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);
            vkCmdDrawIndexed(cmd, submesh.indexCount, 1, submesh.firstIndex, 0, 0);
        }
    }

    VkPipelineLayout DefaultPass::CreateLayout()
    {
        VkPushConstantRange matrixRange;
        matrixRange.offset = 0;
        matrixRange.size = sizeof(PushConstants);
        matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        materialLayout = shader::CreateSetLayout_1(device);

        const VkDescriptorSetLayout layouts[] = {engine->commonSets.GPUSceneData.set, materialLayout.set};

        auto mesh_layout_info = vkinit::New<VkPipelineLayoutCreateInfo>();
        {
            mesh_layout_info.setLayoutCount = sizeof(layouts) / sizeof(VkDescriptorSetLayout);
            mesh_layout_info.pSetLayouts = layouts;
            mesh_layout_info.pPushConstantRanges = &matrixRange;
            mesh_layout_info.pushConstantRangeCount = 1;
        }

        VkPipelineLayout newLayout;
        VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &newLayout));

        return newLayout;
    }

    VkPipeline DefaultPass::CreatePipeline(const VkPipelineLayout pipelineLayout)
    {
        // TODO - Assets::Load<VkShaderModule> returns modules + metadata

        assets_system::AssetFile vertAsset = assets_system::AssetManager::LoadAsset(shader::vertex_asset);
        assets_system::AssetFile fragAsset = assets_system::AssetManager::LoadAsset(shader::pixel_asset);

        const VkShaderModule meshVertexShader = vkutil::load_shader_module(vertAsset.blob, device);
        const VkShaderModule meshFragShader = vkutil::load_shader_module(fragAsset.blob, device);

        nlohmann::json vertMetadata = nlohmann::json::parse(vertAsset.json);
        nlohmann::json fragMetadata = nlohmann::json::parse(fragAsset.json);

        std::string vertEntry = vertMetadata["entry"];
        std::string fragEntry = fragMetadata["entry"];

        PipelineBuilder pipelineBuilder;
        pipelineBuilder.set_shaders(meshVertexShader, meshFragShader, vertEntry.c_str(), fragEntry.c_str());
        pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.set_multisampling_none();
        pipelineBuilder.disable_blending();
        pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

        //render format
        pipelineBuilder.set_color_attachment_format(engine->drawImage.imageFormat);
        pipelineBuilder.set_depth_format(engine->depthImage.imageFormat);

        pipelineBuilder.pipelineLayout = pipelineLayout;

        // finally build the pipeline
        const VkPipeline pipeline = pipelineBuilder.build_pipeline(device);

        vkDestroyShaderModule(device, meshFragShader, nullptr);
        vkDestroyShaderModule(device, meshVertexShader, nullptr);

        return pipeline;
    }


    void DefaultPass::Destroy()
    {
        vkDestroyDescriptorSetLayout(device, materialLayout.set, nullptr);
        materialLayout.set = nullptr;
        
        Pass::Destroy();
    }
}
