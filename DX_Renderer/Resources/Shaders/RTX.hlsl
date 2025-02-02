
#define NV_SHADER_EXTN_SLOT u10
#define NV_SHADER_EXTN_REGISTER_SPACE space0
#include "ThirdParty/nvapi/nvHLSLExtns.h"

struct BuiltinIntersectionAttribs
{ // Barycentric coordinates of hit in
    float2 barycentrics; // the triangle are: (1-x-y, x, y)
};

cbuffer RTCBuffer : register(b0)
{
    float4x4 voxel_space_matrix;
    float4 light_color;
    float3 light_position;
    float light_radius;
    float light_extent;
};

RaytracingAccelerationStructure Scene : register(t1);
RWTexture3D<uint> RenderTarget : register(u0);
RWTexture3D<uint> normal_map : register(u1);

// Packs float4 in [0,1] range into [0-255] uint
uint PackFloat4(float4 val)
{
    uint r = round(clamp(val.r, 0.0, 1.0) * 255.0);
    uint g = round(clamp(val.g, 0.0, 1.0) * 255.0);
    uint b = round(clamp(val.b, 0.0, 1.0) * 255.0);
    uint a = round(clamp(val.a, 0.0, 1.0) * 255.0);
    return (
            (uint(a) & 0x000000FF) << 24U |
            (uint(b) & 0x000000FF) << 16U |
            (uint(g) & 0x000000FF) << 8U |
            (uint(r) & 0x000000FF));
}

// Unpacks values and returns float4 in [0,1] range
float4 UnpackFloat4(uint val)
{
    uint r = (val & 0x000000FF);
    uint g = (val & 0x0000FF00) >> 8U;
    uint b = (val & 0x00FF0000) >> 16U;
    uint a = (val & 0xFF000000) >> 24U;
    return float4(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
}

void AverageRGBA8Voxel(RWTexture3D<uint> voxel_map, int3 voxel_coords, float4 val)
{
    uint packed_color = PackFloat4(float4(val.rgb, 1.0/255.0));
    
    uint previousStoredValue = 0;
    uint currentStoredValue;
    
    float4 currValue;
    float3 average;
    uint count;
    
    InterlockedCompareExchange(voxel_map[voxel_coords], previousStoredValue, packed_color, currentStoredValue);
    while (currentStoredValue != previousStoredValue)
    {
        previousStoredValue = currentStoredValue;
        currValue = UnpackFloat4(previousStoredValue);
        
        average = currValue.rgb;
        count = round(currValue.a * 255.0);
        
        average = (average * count + val.rgb) / (count + 1);
        
        packed_color = PackFloat4(float4(average, (count + 1)/255.0));
        InterlockedCompareExchange(voxel_map[voxel_coords], previousStoredValue, packed_color, currentStoredValue);
    }
}

struct RayPayload
{
    float4 color;
    float3 origin;
    float3 direction;
    float distance;
    bool missed;
};

[shader("raygeneration")]
void raygen()
{
    // Initialize the ray payload
    RayPayload payload;
    payload.color = light_color;
    payload.origin = float3(0, 0, 0);
    payload.direction = float3(0, 0, 0);
    payload.missed = false;
    payload.distance = 0;
    
    // Get the location within the dispatched 2D grid of work items
    // (often maps to pixels, so this could represent a pixel coordinate).
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
    
    float rt_res = 512;
    float3 direction = float3(DispatchRaysIndex().xyz) - float3(rt_res / 2, rt_res / 2, rt_res / 2);
    direction = normalize(direction);
    float4 origin = mul(voxel_space_matrix, float4(light_position, 1.0f));
    
    //ray.Origin = (origin.xyz / origin.w) - float3(1, 1, 1);
    RayDesc ray;
    ray.Origin = light_position;
    ray.Direction = direction;
    ray.TMin = 0.02;
    ray.TMax = 100000;
    
    //uint startTime = NvGetSpecial(9);
    TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    //uint endTime = NvGetSpecial(9);
    //uint deltaTime = endTime - startTime;
    
    float light_intensity = 100.0f;
    float4 irradiance_result = float4(0, 0, 0, 0);
    if(!payload.missed)
    {
        //Shade Primary Hit
        float4 hit_pos = float4(payload.origin, 1.0f);
        
        hit_pos = mul(voxel_space_matrix, hit_pos);
        hit_pos.rgb /= hit_pos.w;
        int3 map_pos = int3(hit_pos.x - 1, hit_pos.y - 1, hit_pos.z - 1);
        
        uint packed_normal = normal_map[map_pos];
        
        float4 normal = UnpackFloat4(packed_normal);
        normal.rgb = (normal.rgb * 2) - 1;
        
        float NdotL = saturate(-dot(normalize(direction), normal.rgb));
        
        float t = abs(payload.distance);
        float falloff = (1.0 / abs(t*t));
        
        float3 irradiance = ((light_intensity * NdotL * falloff) * light_color.rgb);
        irradiance_result = float4(irradiance, 1.0f);
        
        AverageRGBA8Voxel(RenderTarget, map_pos, float4((irradiance), 1.0));
    }
    
    if(!payload.missed && false)
    {
    
        float3 reflection_direction = payload.direction;
        payload.color = irradiance_result;
        payload.origin = float3(0, 0, 0);
        payload.direction = float3(0, 0, 0);
        payload.missed = false;
        payload.distance = 0;
    
        ray.Origin = payload.origin;
        ray.Direction = reflection_direction;
        
        TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
        {
            //Shade Secondary Hit
            float4 hit_pos = float4(payload.origin, 1.0f);
        
            hit_pos = mul(voxel_space_matrix, hit_pos);
            hit_pos.rgb /= hit_pos.w;
            int3 map_pos = int3(hit_pos.x - 1, hit_pos.y - 1, hit_pos.z - 1);
        
            uint packed_normal = normal_map[map_pos];
        
            float4 normal = UnpackFloat4(packed_normal);
            normal.rgb = (normal.rgb * 2) - 1;
        
            float NdotL = saturate(-dot(normalize(reflection_direction), normal.rgb));
        
            float t = abs(payload.distance);
            float falloff = (1.0 / abs(t * t));
        
            float3 irradiance = ((light_intensity * NdotL * falloff) * light_color.rgb);
        
            AverageRGBA8Voxel(RenderTarget, map_pos, float4((irradiance), 1.0));
        }
    }
}

[shader("intersection")]
void intersection()
{
}

[shader("miss")]
void miss(inout RayPayload data : SV_RayPayload)
{
    data.missed = true;
}

[shader("anyhit")]
void anyhit(inout RayPayload data, BuiltinIntersectionAttribs hit)
{
    data.color = float4(1.0f, 0, 0.0f, 1.0f);
}

[shader("closesthit")]
void closesthit(inout RayPayload data, in BuiltinIntersectionAttribs hit)
{
    float dist = RayTCurrent();
    float3 ray_origin = WorldRayOrigin();
    float3 ray_dir = WorldRayDirection();
    
    float4 hit_position = float4(ray_origin + mul(ray_dir, dist), 1.0f);
    float4 hit_pos = hit_position;
    hit_pos = mul(voxel_space_matrix, hit_pos);
    hit_pos.rgb /= hit_pos.w;
    
    int3 map_pos = int3(hit_pos.x - 1, hit_pos.y - 1, hit_pos.z - 1);
    uint packed_normal = normal_map[map_pos];
    float4 normal = UnpackFloat4(packed_normal);
    normal.rgb = normal.rgb * 2 - 1;
    
    float3 new_dir = reflect(normalize(ray_dir), normal.rgb);
    data.color = float4(1, 0, 0, 1);
    data.direction = new_dir;
    data.origin = hit_position.rgb;
    data.distance = dist;
}