#include "hlsl_syntax.hlsli"
#include "global.hlsli"

#pragma vertex vertMask
#pragma pixel fragMask
#pragma vertex vertOutline
#pragma pixel fragOutline

cbuffer OutlineData : register(b0, space1)
{
    float4 colour;
    float thickness;
}

struct PushConstants
{
    float4x4 render_matrix;
    uint64_t vertexBufferAddress;
};

[[vk::push_constant]] PushConstants pushConstants;


struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput vertMask(const uint vertexID : SV_VertexID)
{
    VSOutput output;

    Vertex v = vk::RawBufferLoad<Vertex>(pushConstants.vertexBufferAddress + vertexID * sizeof(Vertex));

    const float4 worldPos = mul(pushConstants.render_matrix, float4(v.position, 1.0f));
    output.position = mul(viewproj, worldPos);
    
    return output;
}

float4 fragMask(const VSOutput i) : SV_TARGET
{
    return 0;
}

VSOutput vertOutline(const uint vertexID : SV_VertexID)
{
    VSOutput output;

    Vertex v = vk::RawBufferLoad<Vertex>(pushConstants.vertexBufferAddress + vertexID * sizeof(Vertex));

    const float4 worldPos = mul(pushConstants.render_matrix, float4(v.position + v.normal * 0.1f, 1.0f));
    output.position = mul(viewproj, worldPos);
    
    return output;
}

float4 fragOutline(const VSOutput i) : SV_TARGET
{
    return float4(0, 0.4f, 1, 1);
}