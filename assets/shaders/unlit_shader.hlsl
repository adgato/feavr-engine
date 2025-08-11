#include "hlsl_syntax.hlsli"

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

struct PushConstants
{
    float4x4 render_matrix;
    float4 identifier;
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
    float4 identifier : TEXCOORD0;
};

float4 shuffleFloat4PCG(float4 input) {
    uint4 state = asuint(input);
    
    // PCG-style permutation
    state = state * 1664525u + 1013904223u;
    state.x += state.y * state.w;
    state.y += state.z * state.x;
    state.z += state.x * state.y;
    state.w += state.y * state.z;
    state ^= state >> 16u;
    state.x += state.y * state.w;
    state.y += state.z * state.x;
    state.z += state.x * state.y;
    state.w += state.y * state.z;
    
    return state * (1.0 / 4294967296.0);
}

VSOutput vert(const uint vertexID : SV_VertexID)
{
    VSOutput output;

    Vertex v = vk::RawBufferLoad<Vertex>(pushConstants.vertexBufferAddress + vertexID * sizeof(Vertex));

    const float4 worldPos = mul(pushConstants.render_matrix, float4(v.position, 1.0f));
    output.position = mul(viewproj, worldPos);
    output.identifier = pushConstants.identifier;
    
    return output;
}

float4 frag(const VSOutput i) : SV_TARGET
{
    return shuffleFloat4PCG(i.identifier);
}