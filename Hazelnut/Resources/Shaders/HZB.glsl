// ---------------------------------------
// -- Hazel Engine HZB shader --
// ---------------------------------------
// - Adopted from Unreal's Engine 4 HZB shader 

#version 430 core
#pragma stage : comp

#define MAX_MIP_BATCH_SIZE 4
#define LOCAL_SIZE 8


layout(push_constant) uniform Info
{
    vec2 u_DispatchThreadIdToBufferUV;
    vec2 u_InputViewportMaxBound;
    vec2 u_InvSize;
    int  u_FirstLod;
    bool u_IsFirstPass;
};

layout(binding = 0, r32f) writeonly uniform image2D o_HZB[4];
layout(binding = 1) uniform sampler2D u_InputDepth;
shared float sharedClosestDeviceZ[LOCAL_SIZE * LOCAL_SIZE];

vec4 Gather4(sampler2D tex, vec2 bufferUV, float lod)
{
    vec2 uv[4];
    uv[0] = min(bufferUV + vec2(-0.5f, -0.5f) * u_InvSize, u_InputViewportMaxBound);
    uv[1] = min(bufferUV + vec2( 0.5f, -0.5f) * u_InvSize, u_InputViewportMaxBound);
    uv[2] = min(bufferUV + vec2(-0.5f,  0.5f) * u_InvSize, u_InputViewportMaxBound);
    uv[3] = min(bufferUV + vec2( 0.5f,  0.5f) * u_InvSize, u_InputViewportMaxBound);
    vec4 depth;
    depth.x = textureLod(tex, uv[0], lod).r;
    depth.y = textureLod(tex, uv[1], lod).r;
    depth.z = textureLod(tex, uv[2], lod).r;
    depth.w = textureLod(tex, uv[3], lod).r;
    return depth;
}


uint SignedRightShift(uint x, const int bitshift)
{
    if (bitshift > 0)
    {
        return x << uint(bitshift);
    }
    else if (bitshift < 0)
    {
        return x >> uint(-bitshift);
    }
    return x;
}

ivec2 InitialTilePixelPositionForReduction2x2(const uint tileSizeLog2, uint sharedArrayId)
{
    uint x = 0u;
    uint y = 0u;
    for (uint i = 0u; i < tileSizeLog2; i++)
    {
        const uint destBitId = tileSizeLog2 - 1u - i;
        const uint destBitMask = 1u << destBitId;
        x |= destBitMask & SignedRightShift(sharedArrayId, int(destBitId) - int(i * 2u + 0u));
        y |= destBitMask & SignedRightShift(sharedArrayId, int(destBitId) - int(i * 2u + 1u));
    }
    return ivec2(x, y);
}

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
void main()
{
    ivec2 groupThreadId = InitialTilePixelPositionForReduction2x2(uint(MAX_MIP_BATCH_SIZE - 1), gl_LocalInvocationIndex);
    ivec2 dispatchThreadId = ivec2(LOCAL_SIZE * gl_WorkGroupID) + groupThreadId;
    vec2 bufferUV = (vec2(dispatchThreadId) + vec2(0.5)) * u_DispatchThreadIdToBufferUV.xy;
    vec4 deviceZ = Gather4(u_InputDepth, bufferUV, u_FirstLod - 1);
    float closestDeviceZ = max(max(deviceZ.x, deviceZ.y), max(deviceZ.z, deviceZ.w));
    ivec2 outputPixelPos = dispatchThreadId;
    if(u_IsFirstPass)
    {
        imageStore(o_HZB[0], outputPixelPos, textureLod(u_InputDepth, bufferUV, 0).xxxx);
    }
    else
    {
        imageStore(o_HZB[0], ivec2(outputPixelPos), closestDeviceZ.xxxx);
    }
    sharedClosestDeviceZ[gl_LocalInvocationIndex] = closestDeviceZ;

    barrier();

    for (int mipLevel = 1; mipLevel < MAX_MIP_BATCH_SIZE; mipLevel++)
    {
        const int tileSize = LOCAL_SIZE / (1 << mipLevel);
        const int reduceBankSize = tileSize * tileSize;

        if (gl_LocalInvocationIndex < reduceBankSize)
        {
            vec4 parentDeviceZ;
            parentDeviceZ.x = closestDeviceZ;
            for (uint i = 1u; i < MAX_MIP_BATCH_SIZE; i++)
            {
                // LDS index
                parentDeviceZ[i] = sharedClosestDeviceZ[gl_LocalInvocationIndex + i * reduceBankSize];
            }
            closestDeviceZ = max(max(parentDeviceZ.x, parentDeviceZ.y), max(parentDeviceZ.z, parentDeviceZ.w));
            outputPixelPos = outputPixelPos >> ivec2(1);
            sharedClosestDeviceZ[gl_LocalInvocationIndex] = closestDeviceZ;
            imageStore(o_HZB[mipLevel], outputPixelPos, closestDeviceZ.xxxx);
        }
    }
}


