#pragma once

#include "rendering/engine-assets/ShaderAssetData.h"

class PipelineBuilder
{
    //> pipeline
public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;

    PipelineBuilder() { clear(); }

    void clear();

    VkPipeline build_pipeline(VkDevice device);

    //< pipeline
    void set_shaders(const engine_assets::ShaderAssetData& vertexShader, const engine_assets::ShaderAssetData& fragmentShader);

    void set_input_topology(VkPrimitiveTopology topology);

    void set_polygon_mode(VkPolygonMode mode);

    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    void set_multisampling_none();

    void disable_blending();

    void enable_blending_additive();

    void enable_blending_alphablend();

    void set_color_attachment_format(VkFormat format);

    void set_depth_format(VkFormat format, bool stencilIsDepth);

    void disable_depthtest(VkStencilOpState* stencilOp);

    void enable_depthtest(bool depthWriteEnable, VkCompareOp op, VkStencilOpState* stencilOp);
};
