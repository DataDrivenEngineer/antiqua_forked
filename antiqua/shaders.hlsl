//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerPass : register(b0)
{
    float4x4        viewMatrix;
    float4x4        projectionMatrix;
    float           windowWidth;
    float           windowHeight;

    uint            atlasWidth;
	uint            atlasHeight;
};

cbuffer cbPerObject : register(b1)
{
    float3          rectCenterPositionWorld;
    float           sideLengthW;
    float3          rectColor;
    float           sideLengthH;
};

cbuffer cbPerTile : register(b2)
{
    float3          color;
    float3          originTileCenterPositionWorld;
    uint            tileCountPerSide;
    float           tileSideLength;
};

// Debug texture
Texture2D g_texture    : register(t0);
SamplerState g_sampler : register(s0);

#define NORMALIZE(OldValue, OldMin, NewRange, OldRange, NewMin) ((((OldValue) - (OldMin)) * (NewRange)) / (OldRange)) + (NewMin) 

#define POINT_SIZE_PX 10

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT {
    float3 position     : POSITION;
    float3 color        : COLOR;
};

struct VS_INPUT_TEXT {
    float  offsetX             : OFFSET_X;
    float  offsetY             : OFFSET_Y;
    uint   atlasRowOffset      : ATLAS_ROW_OFFSET;
    uint   atlasColumnOffset   : ATLAS_COLUMN_OFFSET;
    float  defaultGlyphWidth   : DEFAULT_GLYPH_WIDTH;
    float  defaultGlyphHeight  : DEFAULT_GLYPH_HEIGHT;
    float  glyphWidth          : GLYPH_WIDTH;
    float  glyphHeight         : GLYPH_HEIGHT;
    float2 startPositionScreen : START_POSITION;
    float3 fontColor           : FONT_COLOR;
};

struct VS_OUTPUT_TEXT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
    float3 fontColor: COLOR;
};

struct VS_OUTPUT
{
    float4 position     : SV_POSITION;
    float3 color        : COLOR;
};

struct VS_OUTPUT_RECT
{
    float4 position     : SV_POSITION;
    float3 color        : COLOR;
};

struct VS_OUTPUT_TILE
{
    float4 position     : SV_POSITION;
    float3 color        : COLOR;
};

struct VS_OUTPUT_TEXTURE
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT vsPoint(VS_INPUT input, uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0f), transpose(viewMatrix));
    output.position = mul(output.position, transpose(projectionMatrix));

	float pointCenterX = (output.position.x / output.position.w + 1) * 0.5f * windowWidth;
	float pointCenterY = (output.position.y / output.position.w + 1) * 0.5f * windowHeight;

    float2 topLeft = float2(pointCenterX - POINT_SIZE_PX / 2,
                            pointCenterY + POINT_SIZE_PX / 2);
    float2 topRight = float2(pointCenterX + POINT_SIZE_PX / 2,
                             pointCenterY + POINT_SIZE_PX / 2);
    float2 bottomLeft = float2(pointCenterX - POINT_SIZE_PX / 2,
                               pointCenterY - POINT_SIZE_PX / 2);
    float2 bottomRight = float2(pointCenterX + POINT_SIZE_PX / 2,
                                pointCenterY - POINT_SIZE_PX / 2);

    float2 positions[4] = {
                              float2(NORMALIZE(topLeft.x, 0, 2, windowWidth, -1), NORMALIZE(topLeft.y, 0, 2, windowHeight, -1)),
                              float2(NORMALIZE(topRight.x, 0, 2, windowWidth, -1), NORMALIZE(topRight.y, 0, 2, windowHeight, -1)),
                              float2(NORMALIZE(bottomLeft.x, 0, 2, windowWidth, -1), NORMALIZE(bottomLeft.y, 0, 2, windowHeight, -1)),
                              float2(NORMALIZE(bottomRight.x, 0, 2, windowWidth, -1), NORMALIZE(bottomRight.y, 0, 2, windowHeight, -1))
                          };

    output.position = float4(positions[vertexID], 0.1f, 1.0f);
    output.color = input.color;
    
    return output;
}

VS_OUTPUT_TEXT vsText(VS_INPUT_TEXT input, uint vertexID : SV_VertexID)
{
    float baselineX = input.startPositionScreen.x;
    float baselineY = windowHeight - input.startPositionScreen.y;

    float topY         = baselineY + (float)input.glyphHeight + input.offsetY;
    float bottomY      = baselineY + input.offsetY;
    float2 topLeft     = float2(baselineX + input.offsetX, topY);
    float2 topRight    = float2(baselineX + input.offsetX + (float)input.glyphWidth, topY);
    float2 bottomLeft  = float2(baselineX + input.offsetX, bottomY);
    float2 bottomRight = float2(baselineX + input.offsetX + (float)input.glyphWidth, bottomY);

    float2 positions[4] = {
                              float2(NORMALIZE(topLeft.x, 0, 2, windowWidth, -1), NORMALIZE(topLeft.y, 0, 2, windowHeight, -1)),
                              float2(NORMALIZE(topRight.x, 0, 2, windowWidth, -1), NORMALIZE(topRight.y, 0, 2, windowHeight, -1)),
                              float2(NORMALIZE(bottomLeft.x, 0, 2, windowWidth, -1), NORMALIZE(bottomLeft.y, 0, 2, windowHeight, -1)),
                              float2(NORMALIZE(bottomRight.x, 0, 2, windowWidth, -1), NORMALIZE(bottomRight.y, 0, 2, windowHeight, -1))
                          };

    float atlasRowOffsetF    = (float)input.atlasRowOffset;
    float atlasColumnOffsetF = (float)(input.atlasColumnOffset / 4);
    float atlasWidthF        = (float)atlasWidth;
    float atlasHeightF       = (float)atlasHeight;

    float2 uvs[4];
    uvs[2] = float2(atlasColumnOffsetF / atlasWidthF, (atlasRowOffsetF + input.defaultGlyphHeight) / atlasHeightF);
    uvs[0] = float2(atlasColumnOffsetF / atlasWidthF, atlasRowOffsetF / atlasHeightF);
    uvs[1] = float2((atlasColumnOffsetF + input.defaultGlyphWidth) / atlasWidthF, atlasRowOffsetF / atlasHeightF);
    uvs[3] = float2((atlasColumnOffsetF + input.defaultGlyphWidth) / atlasWidthF, (atlasRowOffsetF + input.defaultGlyphHeight) / atlasHeightF);

    VS_OUTPUT_TEXT output;
    output.position  = float4(positions[vertexID], 0.1f, 1.0f);
    output.uv        = uvs[vertexID];
    output.fontColor = input.fontColor;
    
    return output;
}

VS_OUTPUT vs(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0f), transpose(viewMatrix));
    output.position = mul(output.position, transpose(projectionMatrix));
    output.color    = input.color;
    
    return output;
}

VS_OUTPUT_TILE vsTile(uint vertexID : SV_VertexID,
                      uint instanceID : SV_InstanceID)
{
// NOTE(dima): this is to prevent flickering between tiles and sprites
#define Y -0.001f

    float3 topLeftCornerPosWorld;
    topLeftCornerPosWorld.x = originTileCenterPositionWorld.x - tileCountPerSide / 2;
    topLeftCornerPosWorld.y = Y;
    topLeftCornerPosWorld.z = originTileCenterPositionWorld.z + tileCountPerSide / 2;

    uint rowOfTopLeftCornerPos = instanceID / tileCountPerSide;
    uint colOfTopLeftCornerPos = instanceID % tileCountPerSide;

    float4 tileVertexPositionsWorld[4];
    /* NOTE(dima): index 0 - top left corner of a tile
                   index 1 - top right corner of a tile
                   index 2 - bottom left corner of a tile
                   index 3 - bottom right corner of a tile
    */
    tileVertexPositionsWorld[0].x = topLeftCornerPosWorld.x + tileSideLength * colOfTopLeftCornerPos;
    tileVertexPositionsWorld[0].y = Y;
    tileVertexPositionsWorld[0].z = topLeftCornerPosWorld.z - tileSideLength * rowOfTopLeftCornerPos;
    tileVertexPositionsWorld[0].w = 1.0f;

    tileVertexPositionsWorld[1].x = tileVertexPositionsWorld[0].x + tileSideLength;
    tileVertexPositionsWorld[1].y = Y;
    tileVertexPositionsWorld[1].z = tileVertexPositionsWorld[0].z;
    tileVertexPositionsWorld[1].w = 1.0f;

    tileVertexPositionsWorld[2].x = tileVertexPositionsWorld[0].x;
    tileVertexPositionsWorld[2].y = Y;
    tileVertexPositionsWorld[2].z = tileVertexPositionsWorld[0].z - tileSideLength;
    tileVertexPositionsWorld[2].w = 1.0f;

    tileVertexPositionsWorld[3].x = tileVertexPositionsWorld[0].x + tileSideLength;
    tileVertexPositionsWorld[3].y = Y;
    tileVertexPositionsWorld[3].z = tileVertexPositionsWorld[0].z - tileSideLength;
    tileVertexPositionsWorld[3].w = 1.0f;

#undef Y

    VS_OUTPUT_TILE output;
    output.position = mul(tileVertexPositionsWorld[vertexID], transpose(viewMatrix));
    output.position = mul(output.position, transpose(projectionMatrix));
    output.color = color;

    return output;
}

VS_OUTPUT_RECT vsRect(uint vertexID : SV_VertexID)
{
    float4 rectVertexPositionsWorld[4];
    /* NOTE(dima): index 0 - top left corner of a rect
                   index 1 - top right corner of a rect
                   index 2 - bottom left corner of a rect
                   index 3 - bottom right corner of a rect
    */
    rectVertexPositionsWorld[0].x = rectCenterPositionWorld.x - sideLengthW / 2;
    rectVertexPositionsWorld[0].y = 0.0f;
    rectVertexPositionsWorld[0].z = rectCenterPositionWorld.z + sideLengthH / 2;
    rectVertexPositionsWorld[0].w = 1.0f;

    rectVertexPositionsWorld[1].x = rectVertexPositionsWorld[0].x + sideLengthW;
    rectVertexPositionsWorld[1].y = 0.0f;
    rectVertexPositionsWorld[1].z = rectVertexPositionsWorld[0].z;
    rectVertexPositionsWorld[1].w = 1.0f;

    rectVertexPositionsWorld[2].x = rectVertexPositionsWorld[0].x;
    rectVertexPositionsWorld[2].y = 0.0f;
    rectVertexPositionsWorld[2].z = rectVertexPositionsWorld[0].z - sideLengthH;
    rectVertexPositionsWorld[2].w = 1.0f;

    rectVertexPositionsWorld[3].x = rectVertexPositionsWorld[0].x + sideLengthW;
    rectVertexPositionsWorld[3].y = 0.0f;
    rectVertexPositionsWorld[3].z = rectVertexPositionsWorld[0].z - sideLengthH;
    rectVertexPositionsWorld[3].w = 1.0f;

    VS_OUTPUT_RECT output;
    output.position = mul(rectVertexPositionsWorld[vertexID], transpose(viewMatrix));
    output.position = mul(output.position, transpose(projectionMatrix));
    output.color = rectColor;
    
    return output;
}

VS_OUTPUT_TEXTURE vsTextureDebug(uint vertexID : SV_VertexID)
{

    VS_OUTPUT_TEXTURE output;
#define Z 0.1f
    float4 outPositions[4];
#if 0
    outPositions[2] = float4(-0.5f, -0.5f, Z, 1.0f);
    outPositions[0] = float4(-0.5f, 0.5f, Z, 1.0f);
    outPositions[1] = float4(0.5f, 0.5f, Z, 1.0f);
    outPositions[3] = float4(0.5f, -0.5f, Z, 1.0f);
#else
    outPositions[2] = float4(-1.0f, -1.0f, Z, 1.0f);
    outPositions[0] = float4(-1.0f, 1.0f, Z, 1.0f);
    outPositions[1] = float4(1.0f, 1.0f, Z, 1.0f);
    outPositions[3] = float4(1.0f, -1.0f, Z, 1.0f);
#endif
#undef Z

    float2 uvs[4];
    uvs[2] = float2(0.0f, 1.0f);
    uvs[0] = float2(0.0f, 0.0f);
    uvs[1] = float2(1.0f, 0.0f);
    uvs[3] = float2(1.0f, 1.0f);

    output.position = outPositions[vertexID];
    output.uv = uvs[vertexID];

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 psPoint(VS_OUTPUT input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}

float4 ps(VS_OUTPUT input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}

float4 psTile(VS_OUTPUT_TILE input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}

float4 psRect(VS_OUTPUT_RECT input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}

float4 psTextureDebug(VS_OUTPUT_TEXTURE input) : SV_TARGET
{
    float4 color = float4(g_texture.Sample(g_sampler, input.uv).rgb, 1.0f);
    return color;
}

float4 psText(VS_OUTPUT_TEXT input) : SV_TARGET
{
#if 1
    float alpha = g_texture.Sample(g_sampler, input.uv).a;
    clip(alpha - 0.1f);
    return float4(input.fontColor, 1.0f);
#else
    return float4(0.0f, 0.0f, 1.0f, 1.0f);
#endif
}
