//

#include <metal_stdlib>

using namespace metal;

struct VertexIn {
  packed_float3 position;
  packed_float3 color;
};

struct VertexOut {
  float4 position [[position]];
  float3 color;
};

struct Uniforms
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

[[vertex]] VertexOut vertexShader(
							   const constant VertexIn * vertices [[buffer(5)]],
							   const ushort vertexIndex [[vertex_id]],
							   constant const Uniforms &uniforms [[buffer(7)]])
{
    const constant VertexIn & vData = vertices[vertexIndex];

    float4x4 projectionMatrix = uniforms.projectionMatrix;
    float4x4 viewMatrix = uniforms.viewMatrix;

    VertexOut ret
    {
        .position = projectionMatrix * viewMatrix * float4(vData.position, 1.0f),
        .color = vData.color
    };
    return ret;
}

[[fragment]] float4 fragmentShader(VertexOut in [[stage_in]]) {
  return float4(in.color, 1.0f);
}
