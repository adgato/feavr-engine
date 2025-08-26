#include "IdentifyPass.h"

#include "Mesh.h"
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

void IdentifyPass::Draw(const VkCommandBuffer cmd, size_t frame, const GlobalSceneData& pixelSceneData)
{
    for (auto [id, model, pass] : view)
    {
        if (model.meshRef->id == pass.prevMesh)
            continue;
        if (model.meshRef->id < ecs::BadMaxEntity)
            sorter.addQueue.emplace_back(model.meshRef->id, id);
        if (pass.prevMesh < ecs::BadMaxEntity)
            sorter.removeQueue.emplace_back(pass.prevMesh, id);
        pass.prevMesh = model.meshRef->id;
    }
    sorter.Refresh();

    frame %= FRAME_OVERLAP;
    pixelSceneBuffer[frame].Destroy();
    pixelSceneBuffer[frame] = Buffer<GlobalSceneData>::Allocate(renderer.resource, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HostAccess::SEQUENTIAL_WRITE);
    pixelSceneBuffer[frame].Write(&pixelSceneData);

    pixelSceneProperties.AllocateSet(renderer.resource, renderer.frameData[frame].frameAllocator, renderer.commonSets.sceneDataLayout);
    pixelSceneProperties.StageBuffer(shader_layouts::global::SceneData_binding, pixelSceneBuffer[frame]);
    pixelSceneProperties.PerformWrites();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const VkDescriptorSet sets[] = { pixelSceneProperties.descriptorSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, std::size(sets), sets, 0, nullptr);

    ecs::EntityID prevMesh = ecs::BadMaxEntity;
    VkDeviceAddress vertexBufferAddress = 0;
    for (const auto& [mesh, id] : sorter.sorted)
    {
        if (mesh != prevMesh)
        {
            prevMesh = mesh;
            const Mesh& nextMesh = engine.Get<Mesh>(mesh);
            assert(nextMesh.IsValid());
            vertexBufferAddress = nextMesh.vertexBufferAddress;
            vkCmdBindIndexBuffer(cmd, nextMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }

        const auto& [model, pass] = engine.TryGetMany<Model, PassComponent<IdentifyPass>>(id);
        if (model && pass)
        {
            if (model->visible)
            {
                const PushConstants pushConstants {
                    model->transform,
                    vertexBufferAddress,
                    id
                };

                vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

                const auto& submeshes = *pass->submeshes;
                for (size_t i = 0; i < submeshes.size(); ++i)
                    vkCmdDrawIndexed(cmd, submeshes.data()[i].indexCount, 1, submeshes.data()[i].firstIndex, 0, 0);
            }
        } else
            sorter.removeQueue.emplace_back(mesh, id);
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
