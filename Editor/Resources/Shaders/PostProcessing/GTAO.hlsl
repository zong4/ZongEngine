#pragma stage : comp
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XeGTAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>         (\_/)
// Details:         https://github.com/GameTechDev/XeGTAO                                                     (")_(")
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef XE_GTAO_USE_HALF_FLOAT_PRECISION
#define XE_GTAO_USE_HALF_FLOAT_PRECISION 1
#endif
#include <GTAO.slh>
#include <Buffers.hlslh>
#include <Types.hlslh>

// input output textures for the second pass
[[vk::binding(1, 1)]] Texture2D<float4>           u_ViewNormal;   // source normal map
[[vk::binding(2, 1)]] Texture2D<uint>             u_HilbertLut;   // hilbert lookup table
[[vk::combinedImageSampler]] [[vk::binding(3, 1)]] Texture2D<lpfloat> u_HiZDepth;
[[vk::combinedImageSampler]] [[vk::binding(3, 1)]] SamplerState u_samplerPointClamp;
[[vk::binding(4, 1)]] RWTexture2D<uint>           o_AOwBentNormals;   // output AO term (includes bent normals if enabled - packed as R11G11B10 scaled by AO)
[[vk::binding(5, 1)]] RWTexture2D<unorm float>    o_Edges;   // output depth-based edges used by the denoiser

#define XE_GTAO_DEPTH_MIP_LEVELS                    5                   // this one is hard-coded to 5 for now
#define XE_GTAO_NUMTHREADS_X                        16                   // these can be changed
#define XE_GTAO_NUMTHREADS_Y                        16                   // these can be changed

struct GTAOConstants
{
    float2 NDCToViewMul_x_PixelSize;
    float EffectRadius; // world (viewspace) maximum size of the shadow
    float EffectFalloffRange;

    float RadiusMultiplier;
    float FinalValuePower;
    float DenoiseBlurBeta;
    bool HalfRes;

    float SampleDistributionPower;
    float ThinOccluderCompensation;
    float DepthMIPSamplingOffset;
    int NoiseIndex; // frameIndex % 64 if using TAA or 0 otherwise

    float2 HZBUVFactor;
    float ShadowTolerance;
    float Padding;
};
[[vk::push_constant]] ConstantBuffer<GTAOConstants> u_GTAOConsts;



#ifndef XE_GTAO_USE_DEFAULT_CONSTANTS
#define XE_GTAO_USE_DEFAULT_CONSTANTS 0
#endif

// some constants reduce performance if provided as dynamic values; if these constants are not required to be dynamic and they match default values, 
// set XE_GTAO_USE_DEFAULT_CONSTANTS and the code will compile into a more efficient shader
#define XE_GTAO_DEFAULT_RADIUS_MULTIPLIER               (1.457f  )  // allows us to use different value as compared to ground truth radius to counter inherent screen space biases
#define XE_GTAO_DEFAULT_FALLOFF_RANGE                   (0.615f  )  // distant samples contribute less
#define XE_GTAO_DEFAULT_SAMPLE_DISTRIBUTION_POWER       (2.0f    )  // small crevices more important than big surfaces
#define XE_GTAO_DEFAULT_THIN_OCCLUDER_COMPENSATION      (0.0f    )  // the new 'thickness heuristic' approach
#define XE_GTAO_DEFAULT_FINAL_VALUE_POWER               (2.2f    )  // modifies the final ambient occlusion value using power function - this allows some of the above heuristics to do different things
#define XE_GTAO_DEFAULT_DEPTH_MIP_SAMPLING_OFFSET       (3.30f   )  // main trade-off between performance (memory bandwidth) and quality (temporal stability is the first affected, thin objects next)

#define XE_GTAO_PI               	(3.1415926535897932384626433832795)
#define XE_GTAO_PI_HALF             (1.5707963267948966192313216916398)

#if defined(XE_GTAO_FP32_DEPTHS) && XE_GTAO_USE_HALF_FLOAT_PRECISION
#error Using XE_GTAO_USE_HALF_FLOAT_PRECISION with 32bit depths is not supported yet unfortunately (it is possible to apply fp16 on parts not related to depth but this has not been done yet)
#endif 

uint XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(lpfloat4 unpackedInput)
{
    return ((uint(saturate(unpackedInput.x) * (lpfloat)255 + (lpfloat)0.5)) |
        (uint(saturate(unpackedInput.y) * (lpfloat)255 + (lpfloat)0.5) << 8) |
        (uint(saturate(unpackedInput.z) * (lpfloat)255 + (lpfloat)0.5) << 16) |
        (uint(saturate(unpackedInput.w) * (lpfloat)255 + (lpfloat)0.5) << 24));
}


// Inputs are screen XY and viewspace depth, output is viewspace position
float3 XeGTAO_ComputeViewspacePosition(const float2 screenPos, const float viewspaceDepth)
{
    float3 ret;
    ret.xy = mad(u_Camera.NDCToViewMul, screenPos.xy, u_Camera.NDCToViewAdd) * viewspaceDepth;
    ret.z = viewspaceDepth;
    return ret;
}

lpfloat4 XeGTAO_CalculateEdges(const lpfloat centerZ, const lpfloat leftZ, const lpfloat rightZ, const lpfloat topZ, const lpfloat bottomZ)
{
    lpfloat4 edgesLRTB = lpfloat4(leftZ, rightZ, topZ, bottomZ) - (lpfloat)centerZ;

    lpfloat slopeLR = (edgesLRTB.y - edgesLRTB.x) * 0.5;
    lpfloat slopeTB = (edgesLRTB.w - edgesLRTB.z) * 0.5;
    lpfloat4 edgesLRTBSlopeAdjusted = edgesLRTB + lpfloat4(slopeLR, -slopeLR, slopeTB, -slopeTB);
    edgesLRTB = min(abs(edgesLRTB), abs(edgesLRTBSlopeAdjusted));
    return lpfloat4(saturate((1.25 - edgesLRTB / (centerZ * 0.011))));
}

// packing/unpacking for edges; 2 bits per edge mean 4 gradient values (0, 0.33, 0.66, 1) for smoother transitions!
lpfloat XeGTAO_PackEdges(lpfloat4 edgesLRTB)
{
    // integer version:
    // edgesLRTB = saturate(edgesLRTB) * 2.9.xxxx + 0.5.xxxx;
    // return (((uint)edgesLRTB.x) << 6) + (((uint)edgesLRTB.y) << 4) + (((uint)edgesLRTB.z) << 2) + (((uint)edgesLRTB.w));
    // 
    // optimized, should be same as above
    edgesLRTB = round(saturate(edgesLRTB) * 2.9);
    return dot(edgesLRTB, lpfloat4(64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0));
}

// http://h14s.p5r.org/2012/09/0x5f3759df.html, [Drobot2014a] Low Level Optimizations for GCN, https://blog.selfshadow.com/publications/s2016-shading-course/activision/s2016_pbs_activision_occlusion.pdf slide 63
lpfloat XeGTAO_FastSqrt(float x)
{
    return (lpfloat)(asfloat(0x1fbd1df5 + (asint(x) >> 1)));
}
// input [-1, 1] and output [0, PI], from https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
lpfloat XeGTAO_FastACos(lpfloat inX)
{
    const lpfloat PI = 3.141593;
    const lpfloat HALF_PI = 1.570796;
    lpfloat x = abs(inX);
    lpfloat res = -0.156583 * x + HALF_PI;
    res *= XeGTAO_FastSqrt(1.0 - x);
    return (inX >= 0) ? res : PI - res;
}

uint XeGTAO_EncodeVisibilityBentNormal(lpfloat visibility, lpfloat3 bentNormal)
{
    return XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(lpfloat4(bentNormal * 0.5 + 0.5, visibility));
}

void XeGTAO_OutputWorkingTerm(const uint2 pixCoord, lpfloat visibility, lpfloat3 bentNormal, RWTexture2D<uint> o_AOwBentNormals)
{
    visibility = (lpfloat)saturate(visibility / XE_GTAO_OCCLUSION_TERM_SCALE);
#if __HZ_GTAO_COMPUTE_BENT_NORMALS
    o_AOwBentNormals[pixCoord] = XeGTAO_EncodeVisibilityBentNormal(visibility, bentNormal);
#else
    o_AOwBentNormals[pixCoord] = uint(visibility * 255.0 + 0.5);
#endif
}

// "Efficiently building a matrix to rotate one vector to another"
// http://cs.brown.edu/research/pubs/pdfs/1999/Moller-1999-EBA.pdf / https://dl.acm.org/doi/10.1080/10867651.1999.10487509
// (using https://github.com/assimp/assimp/blob/master/include/assimp/matrix3x3.inl#L275 as a code reference as it seems to be best)
lpfloat3x3 XeGTAO_RotFromToMatrix(lpfloat3 from, lpfloat3 to)
{
    const lpfloat e = dot(from, to);
    const lpfloat f = abs(e); //(e < 0)? -e:e;

    // WARNING: This has not been tested/worked through, especially not for 16bit floats; seems to work in our special use case (from is always {0, 0, -1}) but wouldn't use it in general
    if (f > lpfloat(1.0 - 0.0003))
        return lpfloat3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);

    const lpfloat3 v = cross(from, to);
    /* ... use this hand optimized version (9 mults less) */
    const lpfloat h = (1.0) / (1.0 + e);      /* optimization by Gottfried Chen */
    const lpfloat hvx = h * v.x;
    const lpfloat hvz = h * v.z;
    const lpfloat hvxy = hvx * v.y;
    const lpfloat hvxz = hvx * v.z;
    const lpfloat hvyz = hvz * v.y;

    lpfloat3x3 mtx;
    mtx[0][0] = e + hvx * v.x;
    mtx[0][1] = hvxy - v.z;
    mtx[0][2] = hvxz + v.y;

    mtx[1][0] = hvxy + v.z;
    mtx[1][1] = e + h * v.y * v.y;
    mtx[1][2] = hvyz - v.x;

    mtx[2][0] = hvxz - v.y;
    mtx[2][1] = hvyz + v.x;
    mtx[2][2] = e + hvz * v.z;

    return mtx;
}

float LinearizeDepth(const float screenDepth)
{
    float depthLinearizeMul = u_Camera.DepthUnpackConsts.x;
    float depthLinearizeAdd = u_Camera.DepthUnpackConsts.y;
    // Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"
    return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

float4 LinearizeDepth(const float4 screenDepths)
{
    return float4(LinearizeDepth(screenDepths.x), LinearizeDepth(screenDepths.y), LinearizeDepth(screenDepths.z), LinearizeDepth(screenDepths.w));
}

void XeGTAO_MainPass(const int2 outputPixCoord, const int2 inputPixCoords, lpfloat sliceCount, lpfloat stepsPerSlice, const lpfloat2 localNoise)
{
    lpfloat4 viewspaceNormalLuminance = (lpfloat4)u_ViewNormal.Load(int3(inputPixCoords, 0));
    lpfloat3 viewspaceNormal = viewspaceNormalLuminance.xyz;
    viewspaceNormal.yz = -viewspaceNormalLuminance.yz;


    float2 normalizedScreenPos = (inputPixCoords + 0.5.xx) * u_ScreenData.InvFullResolution;

    float4 deviceZs = u_HiZDepth.GatherRed(u_samplerPointClamp, float2(inputPixCoords * u_ScreenData.InvFullResolution * u_GTAOConsts.HZBUVFactor));
    lpfloat4 valuesUL = (lpfloat4)LinearizeDepth(deviceZs);
    lpfloat4 valuesBR = (lpfloat4)LinearizeDepth(u_HiZDepth.GatherRed(u_samplerPointClamp, float2(inputPixCoords * u_ScreenData.InvFullResolution * u_GTAOConsts.HZBUVFactor), int2(1, 1)));

    // viewspace Z at the center
    lpfloat viewspaceZ = valuesUL.y; //u_HiZDepth.SampleLevel( u_samplerPointClamp, normalizedScreenPos, 0 ).x; 

    // viewspace Zs left top right bottom
    const lpfloat pixLZ = valuesUL.x;
    const lpfloat pixTZ = valuesUL.z;
    const lpfloat pixRZ = valuesBR.z;
    const lpfloat pixBZ = valuesBR.x;

    lpfloat4 edgesLRTB = XeGTAO_CalculateEdges((lpfloat)viewspaceZ, (lpfloat)pixLZ, (lpfloat)pixRZ, (lpfloat)pixTZ, (lpfloat)pixBZ);
    o_Edges[outputPixCoord] = XeGTAO_PackEdges(edgesLRTB);

    // Move center pixel slightly towards camera to avoid imprecision artifacts due to depth buffer imprecision; offset depends on depth texture format used
#ifdef XE_GTAO_FP32_DEPTHS
    viewspaceZ *= 0.99999;     // this is good for FP32 depth buffer
#else
    viewspaceZ *= 0.99920;     // this is good for FP16 depth buffer
#endif

    const float3 pixCenterPos = XeGTAO_ComputeViewspacePosition(normalizedScreenPos, viewspaceZ);
    const lpfloat3 viewVec = (lpfloat3)normalize(-pixCenterPos);

    // prevents normals that are facing away from the view vector - xeGTAO struggles with extreme cases, but in Vanilla it seems rare so it's disabled by default
    // viewspaceNormal = normalize( viewspaceNormal + max( 0, -dot( viewspaceNormal, viewVec ) ) * viewVec );

#if XE_GTAO_USE_DEFAULT_CONSTANTS
    const lpfloat effectRadius = (lpfloat)u_GTAOConsts.EffectRadius * (lpfloat)XE_GTAO_DEFAULT_RADIUS_MULTIPLIER;
    const lpfloat sampleDistributionPower = (lpfloat)XE_GTAO_DEFAULT_SAMPLE_DISTRIBUTION_POWER;
    const lpfloat thinOccluderCompensation = (lpfloat)XE_GTAO_DEFAULT_THIN_OCCLUDER_COMPENSATION;
    const lpfloat falloffRange = (lpfloat)XE_GTAO_DEFAULT_FALLOFF_RANGE * effectRadius;
#else
    const lpfloat effectRadius = (lpfloat)u_GTAOConsts.EffectRadius * (lpfloat)u_GTAOConsts.RadiusMultiplier;
    const lpfloat sampleDistributionPower = (lpfloat)u_GTAOConsts.SampleDistributionPower;
    const lpfloat thinOccluderCompensation = (lpfloat)u_GTAOConsts.ThinOccluderCompensation;
    const lpfloat falloffRange = (lpfloat)u_GTAOConsts.EffectFalloffRange * effectRadius;
#endif

    const lpfloat falloffFrom = effectRadius * ((lpfloat)1 - (lpfloat)u_GTAOConsts.EffectFalloffRange);

    // fadeout precompute optimisation
    const lpfloat falloffMul = (lpfloat)-1.0 / (falloffRange);
    const lpfloat falloffAdd = falloffFrom / (falloffRange)+(lpfloat)1.0;

    lpfloat visibility = 0;
#if __HZ_GTAO_COMPUTE_BENT_NORMALS
    lpfloat3 bentNormal = 0;
#else
    lpfloat3 bentNormal = viewspaceNormal;
#endif

    // see "Algorithm 1" in https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
    {
        const lpfloat noiseSlice = (lpfloat)localNoise.x;
        const lpfloat noiseSample = (lpfloat)localNoise.y;

        // quality settings / tweaks / hacks
        const lpfloat pixelTooCloseThreshold = 1.3;      // if the offset is under approx pixel size (pixelTooCloseThreshold), push it out to the minimum distance

        // approx viewspace pixel size at inputPixCoords; approximation of NDCToViewspace( normalizedScreenPos.xy + u_ScreenData.InvFullResolution.xy, pixCenterPos.z ).xy - pixCenterPos.xy;
        const float2 pixelDirRBViewspaceSizeAtCenterZ = viewspaceZ.xx * u_GTAOConsts.NDCToViewMul_x_PixelSize;

        lpfloat screenspaceRadius = effectRadius / (lpfloat)pixelDirRBViewspaceSizeAtCenterZ.x;

        // fade out for small screen radii 
        visibility += saturate((10 - screenspaceRadius) / 100) * 0.5;

#if 1   // sensible early-out for even more performance; disabled because not yet tested
        lpfloat normals = (lpfloat)viewspaceNormal;
        [branch]
        if (!deviceZs.y || screenspaceRadius < pixelTooCloseThreshold)
        {
            XeGTAO_OutputWorkingTerm(outputPixCoord, 1, viewspaceNormal, o_AOwBentNormals);
            return;
        }
#endif

        // this is the min distance to start sampling from to avoid sampling from the center pixel (no useful data obtained from sampling center pixel)
        const lpfloat minS = (lpfloat)pixelTooCloseThreshold / screenspaceRadius;

        [unroll]
        for (lpfloat slice = 0; slice < sliceCount; slice++)
        {
            lpfloat sliceK = (slice + noiseSlice) / sliceCount;
            // lines 5, 6 from the paper
            lpfloat phi = sliceK * XE_GTAO_PI;
            lpfloat cosPhi = cos(phi);
            lpfloat sinPhi = sin(phi);
            lpfloat2 omega = lpfloat2(cosPhi, -sinPhi);       //lpfloat2 on omega causes issues with big radii

            // convert to screen units (pixels) for later use
            omega *= screenspaceRadius;

            // line 8 from the paper
            const lpfloat3 directionVec = lpfloat3(cosPhi, sinPhi, 0);

            // line 9 from the paper
            const lpfloat3 orthoDirectionVec = directionVec - (dot(directionVec, viewVec) * viewVec);

            // line 10 from the paper
            //axisVec is orthogonal to directionVec and viewVec, used to define projectedNormal
            const lpfloat3 axisVec = normalize(cross(orthoDirectionVec, viewVec));

            // alternative line 9 from the paper
            // float3 orthoDirectionVec = cross( viewVec, axisVec );

            // line 11 from the paper
            lpfloat3 projectedNormalVec = viewspaceNormal - axisVec * dot(viewspaceNormal, axisVec);

            // line 13 from the paper
            lpfloat signNorm = (lpfloat)sign(dot(orthoDirectionVec, projectedNormalVec));

            // line 14 from the paper
            lpfloat projectedNormalVecLength = length(projectedNormalVec);
            lpfloat cosNorm = (lpfloat)saturate(dot(projectedNormalVec, viewVec) / projectedNormalVecLength);

            // line 15 from the paper
            lpfloat n = signNorm * XeGTAO_FastACos(cosNorm);

            // this is a lower weight target; not using -1 as in the original paper because it is under horizon, so a 'weight' has different meaning based on the normal
            const lpfloat lowHorizonCos0 = cos(n + XE_GTAO_PI_HALF);
            const lpfloat lowHorizonCos1 = cos(n - XE_GTAO_PI_HALF);

            // lines 17, 18 from the paper, manually unrolled the 'side' loop
            lpfloat horizonCos0 = lowHorizonCos0; //-1;
            lpfloat horizonCos1 = lowHorizonCos1; //-1;

            [unroll]
            for (lpfloat step = 0; step < stepsPerSlice; step++)
            {
                // R1 sequence (http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/)
                const lpfloat stepBaseNoise = lpfloat(slice + step * stepsPerSlice) * 0.6180339887498948482; // <- this should unroll
                lpfloat stepNoise = frac(noiseSample + stepBaseNoise);

                // approx line 20 from the paper, with added noise
                lpfloat s = (step + stepNoise) / (stepsPerSlice); // + (lpfloat2)1e-6f);

                // additional distribution modifier
                s = (lpfloat)pow(s, (lpfloat)sampleDistributionPower);

                // avoid sampling center pixel
                s += minS;

                // approx lines 21-22 from the paper, unrolled
                lpfloat2 sampleOffset = s * omega;

                lpfloat sampleOffsetLength = length(sampleOffset);

                // note: when sampling, using point_point_point or point_point_linear sampler works, but linear_linear_linear will cause unwanted interpolation between neighbouring depth values on the same MIP level!
                const lpfloat mipLevel = (lpfloat)clamp(log2(sampleOffsetLength) - u_GTAOConsts.DepthMIPSamplingOffset, 0, XE_GTAO_DEPTH_MIP_LEVELS);

                // Snap to pixel center (more correct direction math, avoids artifacts due to sampling pos not matching depth texel center - messes up slope - but adds other 
                // artifacts due to them being pushed off the slice). Also use full precision for high res cases.
                sampleOffset = round(sampleOffset) * (lpfloat2)u_ScreenData.InvFullResolution;

                float2 sampleScreenPos0 = normalizedScreenPos + sampleOffset;
                float  SZ0 = LinearizeDepth(u_HiZDepth.SampleLevel(u_samplerPointClamp, sampleScreenPos0 * u_GTAOConsts.HZBUVFactor, mipLevel).x);
                float3 samplePos0 = XeGTAO_ComputeViewspacePosition(sampleScreenPos0, SZ0);

                float2 sampleScreenPos1 = normalizedScreenPos - sampleOffset;
                float  SZ1 = LinearizeDepth(u_HiZDepth.SampleLevel(u_samplerPointClamp, sampleScreenPos1 * u_GTAOConsts.HZBUVFactor, mipLevel).x);
                float3 samplePos1 = XeGTAO_ComputeViewspacePosition(sampleScreenPos1, SZ1);

                float3 sampleDelta0 = (samplePos0 - float3(pixCenterPos)); // using lpfloat for sampleDelta causes precision issues
                float3 sampleDelta1 = (samplePos1 - float3(pixCenterPos)); // using lpfloat for sampleDelta causes precision issues
                lpfloat sampleDist0 = (lpfloat)length(sampleDelta0);
                lpfloat sampleDist1 = (lpfloat)length(sampleDelta1);

                // approx lines 23, 24 from the paper, unrolled
                lpfloat3 sampleHorizonVec0 = (lpfloat3)(sampleDelta0 / sampleDist0);
                lpfloat3 sampleHorizonVec1 = (lpfloat3)(sampleDelta1 / sampleDist1);

                // any sample out of radius should be discarded - also use fallof range for smooth transitions; this is a modified idea from "4.3 Implementation details, Bounding the sampling area"
#if XE_GTAO_USE_DEFAULT_CONSTANTS != 0 && XE_GTAO_DEFAULT_THIN_OBJECT_HEURISTIC == 0
                lpfloat weight0 = saturate(sampleDist0 * falloffMul + falloffAdd);
                lpfloat weight1 = saturate(sampleDist1 * falloffMul + falloffAdd);
#else
                // this is our own thickness heuristic that relies on sooner discarding samples behind the center
                lpfloat falloffBase0 = length(lpfloat3(sampleDelta0.x, sampleDelta0.y, sampleDelta0.z * (1 + thinOccluderCompensation)));
                lpfloat falloffBase1 = length(lpfloat3(sampleDelta1.x, sampleDelta1.y, sampleDelta1.z * (1 + thinOccluderCompensation)));
                lpfloat weight0 = saturate(falloffBase0 * falloffMul + falloffAdd);
                lpfloat weight1 = saturate(falloffBase1 * falloffMul + falloffAdd);
#endif

                // sample horizon cos
                lpfloat shc0 = (lpfloat)dot(sampleHorizonVec0, viewVec);
                lpfloat shc1 = (lpfloat)dot(sampleHorizonVec1, viewVec);

                // discard unwanted samples
                shc0 = lerp(lowHorizonCos0, shc0, weight0); // this would be more correct but too expensive: cos(lerp( acos(lowHorizonCos0), acos(shc0), weight0 ));
                shc1 = lerp(lowHorizonCos1, shc1, weight1); // this would be more correct but too expensive: cos(lerp( acos(lowHorizonCos1), acos(shc1), weight1 ));

                // thickness heuristic - see "4.3 Implementation details, Height-field assumption considerations"
#if 0   // (disabled, not used) this should match the paper
                lpfloat newhorizonCos0 = max(horizonCos0, shc0);
                lpfloat newhorizonCos1 = max(horizonCos1, shc1);
                horizonCos0 = (horizonCos0 > shc0) ? (lerp(newhorizonCos0, shc0, thinOccluderCompensation)) : (newhorizonCos0);
                horizonCos1 = (horizonCos1 > shc1) ? (lerp(newhorizonCos1, shc1, thinOccluderCompensation)) : (newhorizonCos1);
#elif 0 // (disabled, not used) this is slightly different from the paper but cheaper and provides very similar results
                horizonCos0 = lerp(max(horizonCos0, shc0), shc0, thinOccluderCompensation);
                horizonCos1 = lerp(max(horizonCos1, shc1), shc1, thinOccluderCompensation);
#else   // this is a version where thicknessHeuristic is completely disabled
                horizonCos0 = max(horizonCos0, shc0);
                horizonCos1 = max(horizonCos1, shc1);
#endif
            }

#if 1       // I can't figure out the slight overdarkening on high slopes, so I'm adding this fudge - in the training set, 0.05 is close (PSNR 21.34) to disabled (PSNR 21.45)
            projectedNormalVecLength = lerp(projectedNormalVecLength, 1, 0.05);
#endif

            // line ~27, unrolled
            lpfloat h0 = -XeGTAO_FastACos((lpfloat)horizonCos1);
            lpfloat h1 = XeGTAO_FastACos((lpfloat)horizonCos0);
#if 0       // we can skip clamping for a tiny little bit more performance
            h0 = n + clamp(h0 - n, (lpfloat)-XE_GTAO_PI_HALF, (lpfloat)XE_GTAO_PI_HALF);
            h1 = n + clamp(h1 - n, (lpfloat)-XE_GTAO_PI_HALF, (lpfloat)XE_GTAO_PI_HALF);
#endif
            lpfloat iarc0 = ((lpfloat)cosNorm + (lpfloat)2 * (lpfloat)h0 * (lpfloat)sin(n) - (lpfloat)cos((lpfloat)2 * (lpfloat)h0 - n)) / (lpfloat)4;
            lpfloat iarc1 = ((lpfloat)cosNorm + (lpfloat)2 * (lpfloat)h1 * (lpfloat)sin(n) - (lpfloat)cos((lpfloat)2 * (lpfloat)h1 - n)) / (lpfloat)4;
            lpfloat localVisibility = (lpfloat)projectedNormalVecLength * (lpfloat)(iarc0 + iarc1);
            visibility += localVisibility;

#if __HZ_GTAO_COMPUTE_BENT_NORMALS
            // see "Algorithm 2 Extension that computes bent normals b."
            lpfloat t0 = (6 * sin(h0 - n) - sin(3 * h0 - n) + 6 * sin(h1 - n) - sin(3 * h1 - n) + 16 * sin(n) - 3 * (sin(h0 + n) + sin(h1 + n))) / 12;
            lpfloat t1 = (-cos(3 * h0 - n) - cos(3 * h1 - n) + 8 * cos(n) - 3 * (cos(h0 + n) + cos(h1 + n))) / 12;
            lpfloat3 localBentNormal = lpfloat3(directionVec.x * (lpfloat)t0, directionVec.y * (lpfloat)t0, -lpfloat(t1));
            localBentNormal = (lpfloat3)mul(XeGTAO_RotFromToMatrix(lpfloat3(0, 0, -1), viewVec), localBentNormal) * projectedNormalVecLength;
            bentNormal += localBentNormal;
#endif
        }
        visibility /= (lpfloat)sliceCount;
        visibility = (lpfloat)pow(visibility, (lpfloat)u_GTAOConsts.FinalValuePower * lerp(1.0f, (lpfloat)u_GTAOConsts.ShadowTolerance, viewspaceNormalLuminance.a));
        visibility = max((lpfloat)0.03, visibility); // disallow total occlusion (which wouldn't make any sense anyhow since pixel is visible but also helps with packing bent normals)

#if __HZ_GTAO_COMPUTE_BENT_NORMALS
        bentNormal = normalize(bentNormal);
#endif
    }

    XeGTAO_OutputWorkingTerm(outputPixCoord, visibility, bentNormal, o_AOwBentNormals);
}

// Engine-specific screen & temporal noise loader
lpfloat2 SpatioTemporalNoise(uint2 pixCoord, uint temporalIndex)    // without TAA, temporalIndex is always 0
{
    float2 noise;
    // Hilbert curve driving R2 (see https://www.shadertoy.com/view/3tB3z3)
    uint index = u_HilbertLut.Load(uint3(pixCoord % 64, 0)).x;
    index += 288 * (temporalIndex % 64); // why 288? tried out a few and that's the best so far (with XE_HILBERT_LEVEL 6U) - but there's probably better :)
    // R2 sequence - see http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    return lpfloat2(frac(0.5 + index * float2(0.75487766624669276005, 0.5698402909980532659114)));
}

// Engine-specific entry point for the second pass
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(const uint2 pixCoord : SV_DispatchThreadID)
{
    const int2 outputPixCoords = pixCoord;
    const int2 inputPixCoords = outputPixCoords * (1 + int(u_GTAOConsts.HalfRes));
    XeGTAO_MainPass(outputPixCoords, inputPixCoords, 9, 3, SpatioTemporalNoise(inputPixCoords, u_GTAOConsts.NoiseIndex));
}