cbuffer ConversionMatrixBuffer : register(b0)
{
    float4x4 ClipSpaceMatrix;
    float4x4 VoxelSpaceMatrix;
};

cbuffer MaterialProperties : register(b1)
{
    float3 ambient_coefficient;
    float3 diffuse_coefficient;
    float3 specular_coefficient;
    float specular_exponent;
}

RWTexture3D<uint> albedo_map : register(u0);
RWTexture3D<uint> ocupancy_map : register(u1);
RWTexture3D<uint> diffuse_map : register(u2);
RWTexture3D<uint> specular_map : register(u3);
RWTexture3D<uint> exponent_map : register(u4);
RWTexture3D<uint> normal_map : register(u5);

Texture2D gText : register(t0);
SamplerState gsampler : register(s0);

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 voxel_grip_position : VOXEL_POS;
    float4 normal : NORMAL;
    float2 uv : UV;
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv : UV;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

VS_OUTPUT VoxelVSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = mul(ClipSpaceMatrix, float4(input.pos, 1.0f));
    output.voxel_grip_position = mul(VoxelSpaceMatrix, float4(input.pos, 1.0f));
    output.uv = input.uv;
    //output.normal = mul(VoxelSpaceMatrix, float4(input.normal, 1.0f));
    
    return output;
}

struct PS_OUTPUT
{
    float4 color : SV_TARGET;
};

uint Float4ToRGBA8Uint(float4 val)
{
    return (uint(val.w) & 0x000000FF) << 24U | (uint(val.z) & 0x000000FF) << 16U | (uint(val.y) & 0x000000FF) << 8U | (uint(val.x) & 0x000000FF);
}

float4 RGBA8UintToFloat4(uint val)
{
    return float4(float((val & 0x000000FF)), float((val & 0x0000FF00) >> 8U), float((val & 0x00FF0000) >> 16U), float((val & 0xFF000000) >> 24U));
}

void AverageRGBA8Voxel(RWTexture3D<uint> voxel_map, int3 voxel_coords, float4 val)
{
    val.rgb *= 255.0;
    uint packed_color = Float4ToRGBA8Uint(val);
    uint previousStoredValue = 0;
    uint currentStoredValue;
    
    InterlockedCompareExchange(voxel_map[voxel_coords], previousStoredValue, packed_color, currentStoredValue);
    while (currentStoredValue != previousStoredValue)
    {
        previousStoredValue = currentStoredValue;
        float4 rval = RGBA8UintToFloat4(currentStoredValue);
        rval.rgb = (rval.rgb * rval.a); // Denormalize
        float4 curValF = rval + val; // Add
        curValF.rgb /= curValF.a; // Renormalize
        packed_color = Float4ToRGBA8Uint(curValF);
        InterlockedCompareExchange(voxel_map[voxel_coords], previousStoredValue, packed_color, currentStoredValue);
    }
}

PS_OUTPUT VoxelPSMain(VS_OUTPUT input)
{
    PS_OUTPUT output;
    
    float3 gridPos = input.voxel_grip_position.xyz / input.voxel_grip_position.w;
    int3 voxel_pos = int3(gridPos.x -1, gridPos.y -1, gridPos.z -1);
    
    float4 frag_color = gText.Sample(gsampler, input.uv);
    AverageRGBA8Voxel(albedo_map, voxel_pos, frag_color);
    //AverageRGBA8Voxel(diffuse_map, voxel_pos, float4(diffuse_coefficient,1.0f));
    //AverageRGBA8Voxel(specular_map, voxel_pos, float4(specular_coefficient, 1.0f));
    //AverageRGBA8Voxel(normal_map, voxel_pos, input.normal);
    output.color = input.voxel_grip_position / 256.0f;
    discard;
    
    return output;
}