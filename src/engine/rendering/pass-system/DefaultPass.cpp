#include "DefaultPass.h"

#include "Mesh.h"
#include "rendering/RenderingEngine.h"
#include "rendering/utility/Pipelines.h"
#include "shader_descriptors.h"
#include "assets-system/lookup/Asset16.h"
#include "components/Transform.h"
#include "ecs/EngineView.h"
#include "rendering/engine-assets/ShaderAssetData.h"
#include "rendering/resources/RenderingResources.h"

using namespace shader_layouts::default_shader;
using namespace rendering;

void DefaultPass::Init()
{
    device = renderer.resource.device;

    VkPushConstantRange matrixRange;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(PushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    materialLayout = CreateSetLayout_1(device);

    const VkDescriptorSetLayout layouts[] = { renderer.commonSets.sceneDataLayout.set, materialLayout.set };

    auto mesh_layout_info = vkinit::New<VkPipelineLayoutCreateInfo>(); {
        mesh_layout_info.setLayoutCount = std::size(layouts);
        mesh_layout_info.pSetLayouts = layouts;
        mesh_layout_info.pPushConstantRanges = &matrixRange;
        mesh_layout_info.pushConstantRangeCount = 1;
    }

    VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &layout));

    auto vertexShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_default_shader_vert);
    auto fragmentShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_default_shader_frag);

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
    pipelineBuilder.set_color_attachment_format(renderer.drawImage.imageFormat);
    pipelineBuilder.set_depth_format(renderer.depthImage.imageFormat, true);

    pipelineBuilder.pipelineLayout = layout;

    // finally build the pipeline
    pipeline = pipelineBuilder.build_pipeline(device);

    vertexShader.Destroy();
    fragmentShader.Destroy();

    matConstProperty = Buffer<MaterialConstants>::Allocate(renderer.resource, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HostAccess::SEQUENTIAL_WRITE);

    MaterialConstants data;
    data.colorFactors = glm::vec4 { 1, 1, 1, 1 };
    data.metal_rough_factors = glm::vec4 { 1, 0.5, 0, 0 };

    matConstProperty.Write(&data);

    properties.AllocateSet(renderer.resource, *renderer.resource.globalAllocator, materialLayout);
    // default values
    properties.StageBuffer(GLTFMaterialData_binding, matConstProperty);
    // properties.StageImage(default_pass::colorTex_binding, engine->commonTextures.errorCheckerboard);
    // properties.StageSampler(default_pass::colorTexSampler_binding, engine->defaultSamplerNearest);
    properties.PerformWrites();
}

void DefaultPass::Draw(const VkCommandBuffer cmd)
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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const VkDescriptorSet sets[] = { renderer.sceneProperties.descriptorSet, properties.descriptorSet };
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

        const auto& [model, pass] = engine.TryGetMany<Model, PassComponent<DefaultPass>>(id);
        if (model && pass)
        {
            if (model->visible)
            {
                const PushConstants pushConstants {
                    model->transform,
                    vertexBufferAddress
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
