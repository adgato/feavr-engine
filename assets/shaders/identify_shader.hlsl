#include "hlsl_syntax.hlsli"
#include "global.hlsli"

#pragma vertex vert
#pragma pixel frag

struct PushConstants
{
    float4x4 render_matrix;
    uint64_t vertexBufferAddress;
    uint identifier;
};

[[vk::push_constant]] PushConstants pushConstants;

struct VSOutput
{
    float4 position : SV_Position;
    uint identifier : TEXCOORD0;
};

VSOutput vert(const uint vertexID : SV_VertexID)
{
    VSOutput output;

    Vertex v = vk::RawBufferLoad<Vertex>(pushConstants.vertexBufferAddress + vertexID * sizeof(Vertex));

    const float4 worldPos = mul(pushConstants.render_matrix, float4(v.position, 1.0f));
    output.position = mul(viewproj, worldPos);
    output.identifier = pushConstants.identifier;
    
    return output;
}

uint frag(const VSOutput i) : SV_TARGET
{
    return i.identifier;
}