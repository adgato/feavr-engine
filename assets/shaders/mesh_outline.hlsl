#include "hlsl_syntax.hlsli"

#pragma vertex vertMask
#pragma pixel fragMask
#pragma vertex vertOutline
#pragma pixel fragOutline

cbuffer SceneData : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewproj;
    float4 ambientColor;
    float4 sunlightDirection; // w for sun power
    float4 sunlightColor;
}

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

struct Vertex
{
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 color;
};

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
    return float4(1, 0.4f, 0, 1);
}