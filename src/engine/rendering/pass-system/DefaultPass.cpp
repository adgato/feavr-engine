#include "DefaultPass.h"

#include "rendering/VulkanEngine.h"
#include "rendering/utility/Pipelines.h"
#include "shader_descriptors.h"
#include "ecs/EngineAliases.h"
#include "rendering/engine-assets/ShaderAssetData.h"
#include "rendering/resources/EngineResources.h"

namespace rendering::passes
{
    void DefaultPass::Init(VulkanEngine* engine)
    {
        this->engine = engine;
        device = engine->Resource();

        VkPushConstantRange matrixRange;
        matrixRange.offset = 0;
        matrixRange.size = sizeof(PushConstants);
        matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        materialLayout = default_pass::CreateSetLayout_1(device);

        const VkDescriptorSetLayout layouts[] = { engine->commonSets.GPUSceneData.set, materialLayout.set };

        auto mesh_layout_info = vkinit::New<VkPipelineLayoutCreateInfo>(); {
            mesh_layout_info.setLayoutCount = std::size(layouts);
            mesh_layout_info.pSetLayouts = layouts;
            mesh_layout_info.pPushConstantRanges = &matrixRange;
            mesh_layout_info.pushConstantRangeCount = 1;
        }

        VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &layout));

        auto vertexShader = engine_assets::ShaderAssetData::Load(device, default_pass::vertex_asset);
        auto fragmentShader = engine_assets::ShaderAssetData::Load(device, default_pass::pixel_asset);

        // TODO - most of these can be defaults?
        PipelineBuilder pipelineBuilder;
        pipelineBuilder.set_shaders(vertexShader, fragmentShader);
        pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.set_multisampling_none();
        pipelineBuilder.disable_blending();
        pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

        //render format
        pipelineBuilder.set_color_attachment_format(engine->drawImage.imageFormat);
        pipelineBuilder.set_depth_format(engine->depthImage.imageFormat);

        pipelineBuilder.pipelineLayout = layout;

        // finally build the pipeline
        pipeline = pipelineBuilder.build_pipeline(device);

        vertexShader.Destroy();
        fragmentShader.Destroy();

        matConstProperty = Buffer<MaterialConstants>::Allocate(engine->Resource(), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HostAccess::SEQUENTIAL_WRITE);

        MaterialConstants data;
        data.colorFactors = glm::vec4 { 1, 1, 1, 1 };
        data.metal_rough_factors = glm::vec4 { 1, 0.5, 0, 0 };

        matConstProperty.Write(&data);

        properties.AllocateSet(engine->Resource(), materialLayout);
        // default values
        properties.StageBuffer(default_pass::GLTFMaterialData_binding, matConstProperty);
        properties.StageImage(default_pass::colorTex_binding, engine->commonTextures.errorCheckerboard);
        properties.StageSampler(default_pass::colorTexSampler_binding, engine->defaultSamplerNearest);
        properties.PerformWrites();
    }

    void DefaultPass::Draw(const VkCommandBuffer cmd, const std::vector<Mesh>& meshes)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        const VkDescriptorSet sets[] = { engine->gpuSceneDescriptorSet.descriptorSet, properties.descriptorSet };
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, std::size(sets), sets, 0, nullptr);

        for (const auto& [submeshID, passMesh, data] : engine->sys.Get<ecs::PassEntityManager>().View<SubMesh, default_pass::Component>())
        {
            const Mesh& mesh = meshes[passMesh.meshIndex];
            if (!mesh.IsValid())
                continue;

            vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            for (uint32_t i = 0; i < data.entities->size(); ++i)
            {
                ecs::Entity entity = data.entities->data()[i];

                const PushConstants pushConstants {
                    engine->sys.Get<ecs::MainEntityManager>().GetComponent<Transform>(entity).transform, mesh.vertexBufferAddress
                };

                vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);
                vkCmdDrawIndexed(cmd, passMesh.indexCount, 1, passMesh.firstIndex, 0, 0);
            }
        }
    }

    void DefaultPass::Destroy()
    {
        vkDestroyDescriptorSetLayout(device, materialLayout.set, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        matConstProperty.Destroy();

        materialLayout.set = nullptr;
        layout = nullptr;
        pipeline = nullptr;
    }
}
