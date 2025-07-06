#pragma once
#include <DirectXMath.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat3x3.hpp>

namespace mathx
{
    using namespace DirectX;

    using float4 = FXMVECTOR;
    using float3 = FXMVECTOR;
    using float2 = FXMVECTOR;
    using float1 = FXMVECTOR;
    using float4x4 = FXMMATRIX&;
    using float3x3 = FXMMATRIX&;

    constexpr auto zero = XMVECTOR {};
 
    inline float4 XM_CALLCONV load(glm::quat v) { const XMFLOAT4 xmf = { v.x, v.y, v.z, v.w }; return XMLoadFloat4(&xmf); }
    inline float4 XM_CALLCONV load(glm::vec4 v) { const XMFLOAT4 xmf = { v.x, v.y, v.z, v.w }; return XMLoadFloat4(&xmf); }
    inline float3 XM_CALLCONV load(glm::vec3 v) { const XMFLOAT3 xmf = { v.x, v.y, v.z }; return XMLoadFloat3(&xmf); }
    inline float2 XM_CALLCONV load(glm::vec2 v) { const XMFLOAT2 xmf = { v.x, v.y }; return XMLoadFloat2(&xmf); }
    inline float1 XM_CALLCONV load(float v) { return XMLoadFloat(&v); }
    inline float4x4 XM_CALLCONV load(const glm::mat4& m)
    {
        XMFLOAT4X4 xmf;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                xmf.m[row][col] = m[col][row];
        return XMLoadFloat4x4(&xmf);
    }
    inline float3x3 XM_CALLCONV load(const glm::mat3& m)
    {
        XMFLOAT3X3 xmf;
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                xmf.m[row][col] = m[col][row];
        return XMLoadFloat3x3(&xmf);
    }

    inline void XM_CALLCONV store(glm::vec4& dest, float4 v) { XMFLOAT4 xmf; XMStoreFloat4(&xmf, v); dest = { xmf.x, xmf.y, xmf.z, xmf.w }; }
    inline void XM_CALLCONV store(glm::vec3& dest, float3 v) { XMFLOAT3 xmf; XMStoreFloat3(&xmf, v); dest = { xmf.x, xmf.y, xmf.z }; }
    inline void XM_CALLCONV store(glm::vec2& dest, float2 v) { XMFLOAT2 xmf; XMStoreFloat2(&xmf, v); dest = { xmf.x, xmf.y }; }
    inline void XM_CALLCONV store(float& dest, float1 v) { XMStoreFloat(&dest, v); }
    inline void XM_CALLCONV store(glm::mat4& dest, float4x4 v)
    {
        XMFLOAT4X4 xmf;
        XMStoreFloat4x4(&xmf, v);
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                dest[col][row] = xmf.m[row][col];
    }
    inline void XM_CALLCONV store(glm::mat3& dest, float3x3 v)
    {
        XMFLOAT3X3 xmf;
        XMStoreFloat3x3(&xmf, v);
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                dest[col][row] = xmf.m[row][col];
    }
    
    inline XMVECTOR XM_CALLCONV operator+(FXMVECTOR v1, FXMVECTOR v2) { return XMVectorAdd(v1, v2); }
    inline XMVECTOR XM_CALLCONV operator-(FXMVECTOR v1, FXMVECTOR v2) { return XMVectorSubtract(v1, v2); }
    
    inline float4 XM_CALLCONV dot4(float4 v1, float4 v2) { return XMVector4Dot(v1, v2); }
    inline float3 XM_CALLCONV dot3(float3 v1, float3 v2) { return XMVector3Dot(v1, v2); }
    inline float2 XM_CALLCONV dot2(float2 v1, float2 v2) { return XMVector2Dot(v1, v2); }

    inline float3 XM_CALLCONV cross3(float3 v1, float3 v2) { return { XMVector3Cross(v1, v2) }; }
    
    inline float4 XM_CALLCONV mul4(float4x4 m, float4 v) { return { XMVector4Transform(v, m) }; }
    inline float3 XM_CALLCONV mul3(float3x3 m, float3 v) { return { XMVector3Transform(v, m) }; }
}

