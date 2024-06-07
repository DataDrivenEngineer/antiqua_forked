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
    float pointSize [[point_size]];
};

struct Uniforms
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

vertex VertexOut vertexShader(
							   const constant VertexIn * vertices [[buffer(5)]],
							   const ushort vertexIndex [[vertex_id]],
							   constant const Uniforms &uniforms [[buffer(7)]],
							   constant const float *mousePos [[buffer(8)]])
{
    const constant VertexIn &vData = vertices[vertexIndex];

    float4x4 worldMatrix = uniforms.worldMatrix;
    float4x4 viewMatrix = uniforms.viewMatrix;
    float4x4 projectionMatrix = uniforms.projectionMatrix;

    VertexOut ret
    {
        .position = projectionMatrix * viewMatrix * worldMatrix * float4(vData.position, 1.0f),
        .color = vData.color,
        .pointSize = 10.0f
    };
    return ret;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]]) {
  return float4(in.color, 1.0f);
}

struct VertexInTile {
    uint tileCountPerSide;
    float tileSideLength;
    packed_float3 color;
    packed_float3 originTileCenterPositionWorld;
};

vertex VertexOut vertexShaderTile(
							   const constant VertexInTile *data [[buffer(5)]],
							   const ushort vertexIndex [[vertex_id]],
							   constant const Uniforms &uniforms [[buffer(7)]],
							   constant const float *mousePos [[buffer(8)]],
                 uint instanceId [[instance_id]])
{
    VertexInTile tileData = data[0];
    float3 topLeftCornerPosWorld;
    topLeftCornerPosWorld.x = tileData.originTileCenterPositionWorld.x - tileData.tileCountPerSide / 2;
    topLeftCornerPosWorld.y = 0.0f;
    topLeftCornerPosWorld.z = tileData.originTileCenterPositionWorld.z + tileData.tileCountPerSide / 2;

    uint rowOfTopLeftCornerPos = instanceId / tileData.tileCountPerSide;
    uint colOfTopLeftCornerPos = instanceId % tileData.tileCountPerSide;

    float4 tileVertexPositionsWorld[4];
    /* NOTE(dima): index 0 - top left corner of a tile
                   index 1 - top right corner of a tile
                   index 2 - bottom left corner of a tile
                   index 3 - bottom right corner of a tile
    */

    tileVertexPositionsWorld[0].x = topLeftCornerPosWorld.x + tileData.tileSideLength * colOfTopLeftCornerPos;
    tileVertexPositionsWorld[0].y = 0.0f;
    tileVertexPositionsWorld[0].z = topLeftCornerPosWorld.z - tileData.tileSideLength * rowOfTopLeftCornerPos;
    tileVertexPositionsWorld[0].w = 1.0f;

    tileVertexPositionsWorld[1].x = tileVertexPositionsWorld[0].x + tileData.tileSideLength;
    tileVertexPositionsWorld[1].y = 0.0f;
    tileVertexPositionsWorld[1].z = tileVertexPositionsWorld[0].z;
    tileVertexPositionsWorld[1].w = 1.0f;

    tileVertexPositionsWorld[2].x = tileVertexPositionsWorld[0].x;
    tileVertexPositionsWorld[2].y = 0.0f;
    tileVertexPositionsWorld[2].z = tileVertexPositionsWorld[0].z - tileData.tileSideLength;
    tileVertexPositionsWorld[2].w = 1.0f;

    tileVertexPositionsWorld[3].x = tileVertexPositionsWorld[0].x + tileData.tileSideLength;
    tileVertexPositionsWorld[3].y = 0.0f;
    tileVertexPositionsWorld[3].z = tileVertexPositionsWorld[0].z - tileData.tileSideLength;
    tileVertexPositionsWorld[3].w = 1.0f;

    float4x4 worldMatrix = uniforms.worldMatrix;
    float4x4 viewMatrix = uniforms.viewMatrix;
    float4x4 projectionMatrix = uniforms.projectionMatrix;

    VertexOut ret
    {
        .position = projectionMatrix * viewMatrix * worldMatrix * tileVertexPositionsWorld[vertexIndex],
        .color = tileData.color,
        .pointSize = 0.0f
    };
    return ret;
}

fragment float4 fragmentShaderTile(VertexOut in [[stage_in]]) {
  return float4(in.color, 1.0f);
}
