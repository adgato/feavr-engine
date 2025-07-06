#ifndef COMPILING
#include "hlsl_syntax.hlsli"
#endif

#pragma vertex vert
#pragma pixel frag

cbuffer SceneData : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewproj;
    float4 ambientColor;
    float4 sunlightDirection; // w for sun power
    float4 sunlightColor;
}

cbuffer GLTFMaterialData : register(b0, space1)
{
    float4 colorFactors;
    float4 metal_rough_factors;
}

// Textures (equivalent to samplers)
Texture2D colorTex : register(t1, space1);
SamplerState colorTexSampler : register(s2, space1);

Texture2D metalRoughTex : register(t3, space1);
SamplerState metalRoughTexSampler : register(s4, space1);

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
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
};

VSOutput vert(const uint vertexID : SV_VertexID)
{
    VSOutput output;

    Vertex v = vk::RawBufferLoad<Vertex>(pushConstants.vertexBufferAddress + vertexID * sizeof(Vertex));

    const float4 worldPos = mul(pushConstants.render_matrix, float4(v.position, 1.0f));
    output.position = mul(viewproj, worldPos);
    
    output.normal = mul((float3x3)pushConstants.render_matrix, v.normal);
    output.color = v.color.xyz * colorFactors.xyz;
    output.uv = float2(v.uv_x, v.uv_y);
    
    return output;
}

float4 frag(const VSOutput i) : SV_TARGET
{
    float lightValue = max(dot(i.normal, sunlightDirection.xyz), 0.1f);

    float3 color = i.color * colorTex.Sample(colorTexSampler, i.uv).xyz;
    float3 ambient = color * ambientColor.xyz;

    return float4(color * lightValue *  sunlightColor.w + ambient ,1.0f);
}