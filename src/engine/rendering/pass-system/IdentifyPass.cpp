#include "IdentifyPass.h"

#include "rendering/RenderingEngine.h"
#include "rendering/utility/Pipelines.h"
#include "shader_descriptors.h"
#include "assets-system/lookup/Asset17.h"
#include "components/Transform.h"
#include "ecs/EngineView.h"
#include "rendering/engine-assets/ShaderAssetData.h"
#include "rendering/resources/RenderingResources.h"

using namespace rendering;

void IdentifyPass::Init()
{
    device = renderer.resource.device;

    VkPushConstantRange matrixRange;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(PushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    const VkDescriptorSetLayout layouts[] = { renderer.commonSets.sceneDataLayout.set };

    auto mesh_layout_info = vkinit::New<VkPipelineLayoutCreateInfo>(); {
        mesh_layout_info.setLayoutCount = std::size(layouts);
        mesh_layout_info.pSetLayouts = layouts;
        mesh_layout_info.pPushConstantRanges = &matrixRange;
        mesh_layout_info.pushConstantRangeCount = 1;
    }

    VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &layout));

    auto vertexShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_identify_shader_frag);
    auto fragmentShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_identify_shader_vert);

    // TODO - most of these can be defaults?
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(vertexShader, fragmentShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL, nullptr);

    //render format
    pipelineBuilder.set_color_attachment_format(VK_FORMAT_R32_UINT);
    pipelineBuilder.set_depth_format(VK_FORMAT_D32_SFLOAT, false);

    pipelineBuilder.pipelineLayout = layout;

    // finally build the pipeline
    pipeline = pipelineBuilder.build_pipeline(device);

    vertexShader.Destroy();
    fragmentShader.Destroy();
}

void IdentifyPass::IdentifySubMeshesOf(ecs::Entity transform, std::vector<ecs::EntityID>& outSubMeshes)
{
    // clear here to avoid duplicate entries, might not want to clear in future.
    outSubMeshes.clear();
    for (const auto& [submeshID, passMesh, data] : view)
        for (uint32_t i = 0; i < data.transforms->size(); ++i)
        {
            ecs::Entity entity = data.transforms->data()[i];
            if (transform == entity)
            {
                outSubMeshes.emplace_back(submeshID);
                break;
            }
        }
}

void IdentifyPass::Draw(const VkCommandBuffer cmd, size_t frame, const GlobalSceneData& pixelSceneData)
{
    frame %= FRAME_OVERLAP;
    pixelSceneBuffer[frame].Destroy();
    pixelSceneBuffer[frame] = Buffer<GlobalSceneData>::Allocate(renderer.resource, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HostAccess::SEQUENTIAL_WRITE);
    pixelSceneBuffer[frame].Write(&pixelSceneData);

    pixelSceneDescriptor.AllocateSet(renderer.resource, renderer.frameData[frame].frameAllocator, renderer.commonSets.sceneDataLayout);
    pixelSceneDescriptor.StageBuffer(shader_layouts::global::SceneData_binding, pixelSceneBuffer[frame]);
    pixelSceneDescriptor.PerformWrites();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const VkDescriptorSet sets[] = { pixelSceneDescriptor.descriptorSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, std::size(sets), sets, 0, nullptr);

    for (const auto& [submeshID, passMesh, data] : view)
    {
        const Mesh& mesh = engine.Get<Mesh>(*passMesh.mesh);
        assert(mesh.IsValid());

        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        for (uint32_t i = 0; i < data.transforms->size(); ++i)
        {
            ecs::Entity entity = data.transforms->data()[i];

            const PushConstants pushConstants {
                engine.Get<Transform>(entity).transform,
                mesh.vertexBufferAddress,
                entity,
            };

            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);
            vkCmdDrawIndexed(cmd, passMesh.indexCount, 1, passMesh.firstIndex, 0, 0);
        }
    }
}

void IdentifyPass::Destroy()
{
    vkDestroyPipelineLayout(device, layout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        pixelSceneBuffer[i].Destroy();

    layout = nullptr;
    pipeline = nullptr;
}
