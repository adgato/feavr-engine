#include "hlsl_syntax.hlsli"
#include "global.hlsli"

#pragma vertex vert
#pragma pixel frag

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

struct VSOutput
{
    float4 position : SV_Position;
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
    float lightValue = max(dot(i.normal, normalize(float3(0, 1, 0))), 0.1f);

    float3 color = 1;//i.color * colorTex.Sample(colorTexSampler, i.uv).xyz;
    float3 ambient = color * 0.1;

    return float4(color * lightValue + ambient, 1.0f);
}