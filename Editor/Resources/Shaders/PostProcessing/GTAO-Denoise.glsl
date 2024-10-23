///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#version 450 core
#pragma stage : comp
#include <Common.slh>
#include <Buffers.glslh>

layout(set = 1, binding = 0, r32ui) uniform writeonly uimage2D o_AOTerm;
layout(set = 1, binding = 1) uniform sampler2D u_Edges;
layout(set = 1, binding = 2) uniform usampler2D u_AOTerm;
 
layout(push_constant) uniform DenoiseConstants
{
    float DenoiseBlurBeta;
    bool HalfRes;
} u_Settings;

#if __HZ_GTAO_COMPUTE_BENT_NORMALS
#define AOTermType vec4           // .xyz is bent normal, .w is visibility term
#else
#define AOTermType float          // .x is visibility term
#endif


vec4 XeGTAO_R8G8B8A8_UNORM_to_FLOAT4(uint packedInput)
{
    vec4 unpackedOutput;
    unpackedOutput.x = float(packedInput & 0x000000ff) / 255.f;
    unpackedOutput.y = float(((packedInput >> 8) & 0x000000ff)) / 255.f;
    unpackedOutput.z = float(((packedInput >> 16) & 0x000000ff)) / 255.f;
    unpackedOutput.w = float(packedInput >> 24) / 255.f;
    return unpackedOutput;
}

void XeGTAO_DecodeVisibilityBentNormal(const uint packedValue, out float visibility, out vec3 bentNormal)
{
    vec4 decoded = XeGTAO_R8G8B8A8_UNORM_to_FLOAT4(packedValue);
    bentNormal = decoded.xyz * 2.0.xxx - 1.0.xxx;   // could normalize - don't want to since it's done so many times, better to do it at the final step only
    visibility = decoded.w;
}


void XeGTAO_DecodeGatherPartial(const uvec4 packedValue, out AOTermType outDecoded[4])
{
    for( int i = 0; i < 4; i++ )
    {
    #if __HZ_GTAO_COMPUTE_BENT_NORMALS
        XeGTAO_DecodeVisibilityBentNormal(packedValue[i], outDecoded[i].w, outDecoded[i].xyz);
    #else
        outDecoded[i] = float(packedValue[i]) / 255.0;
    #endif
    }
}

vec4 XeGTAO_UnpackEdges(float _packedVal)
{
    uint packedVal = uint(_packedVal * 255.5);
    vec4 edgesLRTB;
    edgesLRTB.x = float((packedVal >> 6) & 0x03) / 3.0;          // there's really no need for mask (as it's an 8 bit input) but I'll leave it in so it doesn't cause any trouble in the future
    edgesLRTB.y = float((packedVal >> 4) & 0x03) / 3.0;
    edgesLRTB.z = float((packedVal >> 2) & 0x03) / 3.0;
    edgesLRTB.w = float((packedVal >> 0) & 0x03) / 3.0;

    return clamp(edgesLRTB, 0.0, 1.0);
}

void XeGTAO_AddSample(AOTermType ssaoValue, float edgeValue, inout AOTermType sum, inout float sumWeight)
{
    float weight = edgeValue;    

    sum += (weight * ssaoValue);
    sumWeight += weight;
}

uint XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(vec4 unpackedInput)
{
    return ((uint(clamp(unpackedInput.x, 0.0, 1.0) * 255.f + 0.5f)) |
            (uint(clamp(unpackedInput.y, 0.0, 1.0) * 255.f + 0.5f) << 8 ) |
            (uint(clamp(unpackedInput.z, 0.0, 1.0) * 255.f + 0.5f) << 16 ) |
            (uint(clamp(unpackedInput.w, 0.0, 1.0) * 255.f + 0.5f) << 24 ) );
}


uint XeGTAO_EncodeVisibilityBentNormal(float visibility, vec3 bentNormal)
{
    return XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(vec4(bentNormal * 0.5 + 0.5, visibility));
}

void XeGTAO_Output(ivec2 pixCoord, AOTermType outputValue)
{
#if __HZ_GTAO_COMPUTE_BENT_NORMALS
    float     visibility = outputValue.w;
    vec3    bentNormal = normalize(outputValue.xyz);
    imageStore(o_AOTerm, pixCoord, uint(XeGTAO_EncodeVisibilityBentNormal(visibility, bentNormal)).xxxx);
#else
    imageStore(o_AOTerm, pixCoord, uint(min(outputValue * 255.0 + 0.5, 255.f)).xxxx);
#endif
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    
    const float blurAmount = (u_Settings.DenoiseBlurBeta / 5.0);
    const float diagWeight = 0.85 * 0.5;

    AOTermType aoTerm[2];   // pixel pixCoordBase and pixel pixCoordBase + ivec2( 1, 0 )
    vec4 edgesC_LRTB[2];
    float weightTL[2];
    float weightTR[2];
    float weightBL[2];
    float weightBR[2];

    // we're computing 2 horizontal pixels at a time (performance optimization)
    ivec2 pixCoordBase = ivec2(gl_GlobalInvocationID.xy * ivec2(2, 1));

    // gather edge and visibility quads, used later
    vec2 gatherCenter = vec2(pixCoordBase) * u_ScreenData.InvFullResolution * (1 + int(u_Settings.HalfRes));

    vec4 edgesQ0        = textureGather(u_Edges, gatherCenter);
    vec4 edgesQ1        = textureGatherOffset(u_Edges, gatherCenter, ivec2(2, 0));
    vec4 edgesQ2        = textureGatherOffset(u_Edges, gatherCenter, ivec2(1, 2));

    AOTermType visQ0[4];    XeGTAO_DecodeGatherPartial(textureGatherOffset(u_AOTerm, gatherCenter, ivec2(0, 0)), visQ0);
    AOTermType visQ1[4];    XeGTAO_DecodeGatherPartial(textureGatherOffset(u_AOTerm, gatherCenter, ivec2(2, 0)), visQ1);
    AOTermType visQ2[4];    XeGTAO_DecodeGatherPartial(textureGatherOffset(u_AOTerm, gatherCenter, ivec2(0, 2)), visQ2);
    AOTermType visQ3[4];    XeGTAO_DecodeGatherPartial(textureGatherOffset(u_AOTerm, gatherCenter, ivec2(2, 2)), visQ3);

    for(int side = 0; side < 2; side++)
    {
        const ivec2 pixCoord = ivec2(pixCoordBase.x + side, pixCoordBase.y);

        vec4 edgesL_LRTB  = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.x) : (edgesQ0.y));
        vec4 edgesT_LRTB  = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.z) : (edgesQ1.w));
        vec4 edgesR_LRTB  = XeGTAO_UnpackEdges((side == 0) ? (edgesQ1.x) : (edgesQ1.y));
        vec4 edgesB_LRTB  = XeGTAO_UnpackEdges((side == 0) ? (edgesQ2.w) : (edgesQ2.z));

        edgesC_LRTB[side] = XeGTAO_UnpackEdges((side == 0) ? edgesQ0.y : edgesQ1.x);

        // Edges aren't perfectly symmetrical: edge detection algorithm does not guarantee that a left edge on the right pixel will match the right edge on the left pixel (although
        // they will match in majority of cases). This line further enforces the symmetricity, creating a slightly sharper blur. Works real nice with TAA.
        edgesC_LRTB[side] *= vec4(edgesL_LRTB.y, edgesR_LRTB.x, edgesT_LRTB.w, edgesB_LRTB.z);

#if 1   // this allows some small amount of AO leaking from neighbours if there are 3 or 4 edges; this reduces both spatial and temporal aliasing
        const float leak_threshold = 2.5; const float leak_strength = 0.5;
        float edginess = (clamp(4.0 - leak_threshold - dot(edgesC_LRTB[side], vec4(1.0)), 0.0, 1.0) / (4 - leak_threshold)) * leak_strength;
        edgesC_LRTB[side] = clamp(edgesC_LRTB[side] + edginess, 0.0, 1.0);
#endif

        // for diagonals; used by first and second pass
        weightTL[side] = diagWeight * (edgesC_LRTB[side].x * edgesL_LRTB.z + edgesC_LRTB[side].z * edgesT_LRTB.x);
        weightTR[side] = diagWeight * (edgesC_LRTB[side].z * edgesT_LRTB.y + edgesC_LRTB[side].y * edgesR_LRTB.z);
        weightBL[side] = diagWeight * (edgesC_LRTB[side].w * edgesB_LRTB.x + edgesC_LRTB[side].x * edgesL_LRTB.w);
        weightBR[side] = diagWeight * (edgesC_LRTB[side].y * edgesR_LRTB.w + edgesC_LRTB[side].w * edgesB_LRTB.y);

        // first pass
        AOTermType ssaoValue     = side == 0 ? visQ0[1] : visQ1[0];
        AOTermType ssaoValueL    = side == 0 ? visQ0[0] : visQ0[1];
        AOTermType ssaoValueT    = side == 0 ? visQ0[2] : visQ1[3];
        AOTermType ssaoValueR    = side == 0 ? visQ1[0] : visQ1[1];
        AOTermType ssaoValueB    = side == 0 ? visQ2[2] : visQ3[3];
        AOTermType ssaoValueTL   = side == 0 ? visQ0[3] : visQ0[2];
        AOTermType ssaoValueBR   = side == 0 ? visQ3[3] : visQ3[2];
        AOTermType ssaoValueTR   = side == 0 ? visQ1[3] : visQ1[2];
        AOTermType ssaoValueBL   = side == 0 ? visQ2[3] : visQ2[2];

        float sumWeight = blurAmount;
        AOTermType sum = ssaoValue * sumWeight;

        XeGTAO_AddSample(ssaoValueL, edgesC_LRTB[side].x, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueR, edgesC_LRTB[side].y, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueT, edgesC_LRTB[side].z, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueB, edgesC_LRTB[side].w, sum, sumWeight);

        XeGTAO_AddSample(ssaoValueTL, weightTL[side], sum, sumWeight);
        XeGTAO_AddSample(ssaoValueTR, weightTR[side], sum, sumWeight);
        XeGTAO_AddSample(ssaoValueBL, weightBL[side], sum, sumWeight);
        XeGTAO_AddSample(ssaoValueBR, weightBR[side], sum, sumWeight);

        aoTerm[side] = sum / sumWeight;

        XeGTAO_Output(pixCoord, aoTerm[side]);
    }
}


