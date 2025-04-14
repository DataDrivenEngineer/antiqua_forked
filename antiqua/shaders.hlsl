//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerPass : register(b0)
{
    float4x4        viewMatrix;
    float4x4        projectionMatrix;
};

cbuffer cbPerObject : register(b1)
{
    float4x4        modelMatrix;
    float4          meshCenterAndMinY;
};

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
float4 ps(VS_OUTPUT input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}

float4 psMesh(VS_OUTPUT_MESH input) : SV_TARGET
{
    return input.color;
}
