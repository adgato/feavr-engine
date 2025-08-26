#include "StencilOutlinePass.h"

#include "Mesh.h"
#include "assets-system/lookup/Asset30.h"
#include "components/Transform.h"
#include "rendering/RenderingEngine.h"
#include "rendering/resources/ResourceHandles.h"
#include "rendering/utility/Pipelines.h"

using namespace rendering;

void StencilOutlinePass::Init()
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

    VkStencilOpState maskStencil {};
    maskStencil.compareOp = VK_COMPARE_OP_ALWAYS;
    maskStencil.passOp = VK_STENCIL_OP_REPLACE;
    maskStencil.failOp = VK_STENCIL_OP_REPLACE;
    maskStencil.compareMask = 1;
    maskStencil.writeMask = 1;
    maskStencil.reference = 1;

    VkStencilOpState outlineStencil {};
    outlineStencil.compareOp = VK_COMPARE_OP_NOT_EQUAL;
    outlineStencil.passOp = VK_STENCIL_OP_KEEP;
    outlineStencil.failOp = VK_STENCIL_OP_KEEP;
    outlineStencil.compareMask = 1;
    outlineStencil.writeMask = 1;
    outlineStencil.reference = 1;

    VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &maskLayout));

    auto vertexMaskShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_mesh_outline_vertMask);
    auto fragmentMaskShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_mesh_outline_fragMask);

    PipelineBuilder maskPipelineBuilder;
    maskPipelineBuilder.set_shaders(vertexMaskShader, fragmentMaskShader);
    maskPipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    maskPipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    maskPipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    maskPipelineBuilder.set_multisampling_none();
    // no `disable_blending` we don't want a colour write mask just writing to stencil with this pass

    // don't write to depth, always pass depth test.
    maskPipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_ALWAYS, &maskStencil);

    //render format
    maskPipelineBuilder.set_color_attachment_format(renderer.drawImage.imageFormat);
    maskPipelineBuilder.set_depth_format(renderer.depthImage.imageFormat, true);

    maskPipelineBuilder.pipelineLayout = maskLayout;

    // finally build the pipeline
    maskPipeline = maskPipelineBuilder.build_pipeline(device);

    vertexMaskShader.Destroy();
    fragmentMaskShader.Destroy();

    VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &outlineLayout));

    auto vertexOutlineShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_mesh_outline_vertOutline);
    auto fragmentOutlineShader = engine_assets::ShaderAssetData::Load(device, assets_system::lookup::SHAD_mesh_outline_fragOutline);

    PipelineBuilder outlinePipelineBuilder;
    outlinePipelineBuilder.set_shaders(vertexOutlineShader, fragmentOutlineShader);
    outlinePipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    outlinePipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    outlinePipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    outlinePipelineBuilder.set_multisampling_none();
    // write colour this pass
    outlinePipelineBuilder.disable_blending();

    // don't write to depth, always pass depth test.
    outlinePipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_ALWAYS, &outlineStencil);

    //render format
    outlinePipelineBuilder.set_color_attachment_format(renderer.drawImage.imageFormat);
    outlinePipelineBuilder.set_depth_format(renderer.depthImage.imageFormat, true);

    outlinePipelineBuilder.pipelineLayout = outlineLayout;

    // finally build the pipeline
    outlinePipeline = outlinePipelineBuilder.build_pipeline(device);

    vertexOutlineShader.Destroy();
    fragmentOutlineShader.Destroy();
}

void StencilOutlinePass::Draw(const VkCommandBuffer cmd)
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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, maskPipeline);

    const VkDescriptorSet sets[] = { renderer.sceneProperties.descriptorSet };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, maskLayout, 0, std::size(sets), sets, 0, nullptr);

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

        const auto& [model, pass] = engine.TryGetMany<Model, PassComponent<StencilOutlinePass>>(id);
        if (model && pass)
        {
            if (model->visible)
            {
                const PushConstants pushConstants {
                    model->transform,
                    vertexBufferAddress
                };

                vkCmdPushConstants(cmd, maskLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

                const auto& submeshes = *pass->submeshes;
                for (size_t i = 0; i < submeshes.size(); ++i)
                    vkCmdDrawIndexed(cmd, submeshes.data()[i].indexCount, 1, submeshes.data()[i].firstIndex, 0, 0);
            }
        } else
            sorter.removeQueue.emplace_back(mesh, id);
    }


    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, outlineLayout, 0, std::size(sets), sets, 0, nullptr);

    prevMesh = ecs::BadMaxEntity;
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

        const auto& [model, pass] = engine.TryGetMany<Model, PassComponent<StencilOutlinePass>>(id);
        if (model && pass)
        {
            if (model->visible)
            {
                const PushConstants pushConstants {
                    model->transform,
                    vertexBufferAddress
                };

                vkCmdPushConstants(cmd, maskLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

                const auto& submeshes = *pass->submeshes;
                for (size_t i = 0; i < submeshes.size(); ++i)
                    vkCmdDrawIndexed(cmd, submeshes.data()[i].indexCount, 1, submeshes.data()[i].firstIndex, 0, 0);
            }
        }
    }
}

void StencilOutlinePass::Destroy()
{
    vkDestroyPipelineLayout(device, maskLayout, nullptr);
    vkDestroyPipelineLayout(device, outlineLayout, nullptr);
    vkDestroyPipeline(device, maskPipeline, nullptr);
    vkDestroyPipeline(device, outlinePipeline, nullptr);

    maskLayout = nullptr;
    outlineLayout = nullptr;
    maskPipeline = nullptr;
    outlinePipeline = nullptr;
}
