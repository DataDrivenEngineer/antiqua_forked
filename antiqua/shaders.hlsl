//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerPass : register(b0)
{
    float4x4        viewMatrix;
    float4x4        projectionMatrix;
    float           windowWidth;
    float           windowHeight;
};

cbuffer cbPerObject : register(b1)
{
    float4x4        modelMatrix;
    float4          meshCenterAndMinY;
};

#define POINT_SIZE_PX 10

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 position     : POSITION;
    float3 color        : COLOR;
};

struct VS_OUTPUT
{
    float4 position     : SV_POSITION;
    float3 color        : COLOR;
};

struct VS_INPUT_MESH
{
    float3 position     : POSITION;
};

struct VS_OUTPUT_MESH
{
    float4 position     : SV_POSITION;
    float4 color        : COLOR;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT vsPoint(VS_INPUT input, uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0f), transpose(viewMatrix));
    output.position = mul(output.position, transpose(projectionMatrix));

#define NORMALIZE(OldValue, OldMin, NewRange, OldRange, NewMin) ((((OldValue) - (OldMin)) * (NewRange)) / (OldRange)) + (NewMin) 

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

VS_OUTPUT vs(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0f), transpose(viewMatrix));
    output.position = mul(output.position, transpose(projectionMatrix));
    output.color = input.color;
    
    return output;
}

VS_OUTPUT_MESH vsMesh(VS_INPUT_MESH input)
{
    VS_OUTPUT_MESH output;

    float meshCenterX = meshCenterAndMinY.x;
    float meshCenterY = meshCenterAndMinY.y;
    float meshCenterZ = meshCenterAndMinY.z;
    float meshMinY = meshCenterAndMinY.w;

    float3 vDataPosModified = float3(input.position);
    
    // NOTE(dima): move mesh center to (0;0;0)
    vDataPosModified.x -= meshCenterX;
    vDataPosModified.y -= meshCenterY;
    vDataPosModified.z -= meshCenterZ;

    /* NOTE(dima): move mesh so that its lowest Y = 0.
                   Basically, it means: put model on the ground */
    vDataPosModified.y += meshMinY;

    // NOTE(dima): convert from right handed to left handed system
    vDataPosModified.z = -vDataPosModified.z;

    output.position = mul(float4(vDataPosModified, 1.0f), transpose(modelMatrix));
    output.position = mul(output.position, transpose(viewMatrix));
    output.position = mul(output.position, transpose(projectionMatrix));

    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
 
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

float4 psMesh(VS_OUTPUT_MESH input) : SV_TARGET
{
    return input.color;
}
