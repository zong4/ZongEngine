// ---------------------------------------
// -- Engine SSR shader --
// ---------------------------------------
// - References:
//      AMD's Stochastic screen-space reflections https://gpuopen.com/fidelityfx-sssr/
//      GPU Pro 5 book chapter 4 by Yasin Uludag
//      Will Pearce's blog http://roar11.com/2015/07/screen-space-glossy-reflections/
//      Unreal Engine 4 SSR implimentation 
//      Lukas Hermanns's thesis https://lukas-hermanns.info/download/bachelorthesis_ssct_lhermanns.pdf
#version 430 core
#pragma stage : comp
#include <Common.slh>
#include <GTAO.slh>

#include <Buffers.glslh>

layout(set = 1, binding = 1, rgba16f) restrict writeonly uniform image2D outColor;
layout(set = 1, binding = 2) uniform sampler2D u_InputColor;
layout(set = 1, binding = 3) uniform sampler2D u_Normal;
layout(set = 1, binding = 4) uniform sampler2D u_HiZBuffer;
layout(set = 1, binding = 5) uniform sampler2D u_MetalnessRoughness;
layout(set = 1, binding = 6) uniform sampler2D u_VisibilityBuffer;

#define GTAO_REFLECTION_OCCLUSION (__ZONG_REFLECTION_OCCLUSION_METHOD & ZONG_REFLECTION_OCCLUSION_METHOD_GTAO)
#define HBAO_REFLECTION_OCCLUSION (__ZONG_REFLECTION_OCCLUSION_METHOD & ZONG_REFLECTION_OCCLUSION_METHOD_HBAO)

#if GTAO_REFLECTION_OCCLUSION
	layout(set = 1, binding = 7) uniform usampler2D u_GTAOTex;
#endif

#if HBAO_REFLECTION_OCCLUSION
	layout(set = 1, binding = 8) uniform sampler2D u_HBAOTex;
#endif

layout(push_constant) uniform SSRInfo
{
	vec2 HZBUVFactor;
	vec2 FadeIn;
	float Brightness;
	float DepthTolerance;
	float FacingReflectionsFading;
	int MaxSteps;
	uint NumDepthMips;
	float RoughnessDepthTolerance; // The higher the roughness the more we have depth tolerance
	bool HalfRes;
	bool EnableConeTracing;
	float LuminanceFactor;
} u_SSRInfo;


#define INVERTED_DEPTH_RANGE
const float M_PI = 3.14159265359f;
const float FLOAT_MAX = 3.402823466e+38f;
const int BASE_LOD = 0;

///////////////////////////////////////////////////////////////////////////////////////
// Hi-Z cone tracing
///////////////////////////////////////////////////////////////////////////////////////

float IsoscelesTriangleOpposite(float adjacentLength, float coneTheta)
{
	return 2.0 * tan(coneTheta) * adjacentLength;
}

float IsoscelesTriangleInRadius(float a, float h)
{
	float a2 = a * a;
	float fh2 = 4.0f * h * h;
	return (a * (sqrt(a2 + fh2) - a)) / (4.0f * h);
}

vec4 ConeSampleWeightedColor(vec2 samplePos, float mipLevel)
{
	/* Sample color buffer with pre-integrated visibility */
	vec3 color = textureLod(u_InputColor, samplePos, mipLevel).rgb;
	float visibility = textureLod(u_VisibilityBuffer, samplePos, mipLevel).r;
	return vec4(color * visibility, visibility);
}

float IsoscelesTriangleNextAdjacent(float adjacentLength, float incircleRadius)
{
	// subtract the diameter of the incircle to get the adjacent side of the next level on the cone
	return adjacentLength - (incircleRadius * 2.0);
}

vec4 ConeTracing(float roughness, vec2 rayOriginSS, vec2 rayPosSS)
{
	float coneTheta = roughness * M_PI * 0.025;

    vec2 res = u_ScreenData.FullResolution * (1 + int(!u_SSRInfo.HalfRes));

	/* Cone tracing using an isosceles triangle to approximate a cone in screen space */
	vec2 deltaPos = rayPosSS - rayOriginSS;


	float adjacentLength = length(deltaPos);
	vec2 adjacentUnit = normalize(deltaPos);

	vec4 reflectionColor = vec4(0.0);
	vec2 samplePos;
	float remainingAlpha = 1.0f;
	for (int i = 0; i < 7; ++i)
	{
		// intersection length is the adjacent side, get the opposite side using trig
		float oppositeLength = IsoscelesTriangleOpposite(adjacentLength, coneTheta);

		// calculate in-radius of the isosceles triangle
		float incircleSize = IsoscelesTriangleInRadius(oppositeLength, adjacentLength);

		// get the sample position in screen space
		samplePos = rayOriginSS + adjacentUnit * (adjacentLength - incircleSize);

		// convert the in-radius into screen size then check what power N to raise 2 to reach it - that power N becomes mip level to sample from
		float mipChannel = clamp(log2(incircleSize * max(res.x, res.y)), 0.0f, u_SSRInfo.NumDepthMips);

		/*
		 * Read color and accumulate it using trilinear filtering and weight it.
		 * Uses pre-convolved image (color buffer) and glossiness to weigh color contributions.
		 * Visibility is accumulated in the alpha channel. Break if visibility is 100% or greater (>= 1.0f).
		 */
		vec4 newColor = ConeSampleWeightedColor(samplePos.xy, mipChannel);
		remainingAlpha -= newColor.a;
		if (remainingAlpha < 0.0f)
		{
			newColor.rgb *= (1.0f - abs(remainingAlpha));
		}
		reflectionColor += newColor;

		if (reflectionColor.a >= 1.0f)
		{
			break;
		}

		adjacentLength = IsoscelesTriangleNextAdjacent(adjacentLength, incircleSize);
	}

	return reflectionColor;
}

///////////////////////////////////////////////////////////////////////////////////////
// Hi-Z ray tracing methods
///////////////////////////////////////////////////////////////////////////////////////

float FetchDepth(ivec2 coords, int lod)
{
	return texelFetch(u_HiZBuffer, coords, lod).x;
}

ivec2 GetDepthMipResolution(int mipLevel)
{
	return ivec2(textureSize(u_HiZBuffer, mipLevel) * u_SSRInfo.HZBUVFactor);
}

// Inputs are screen XY and viewspace depth, output is viewspace position
// From XeGTAO
vec3 ComputeViewspacePosition(const vec2 screenPos, const float viewspaceDepth)
{
	vec3 ret;
	ret.xy = (u_Camera.NDCToViewMul * screenPos.xy + u_Camera.NDCToViewAdd) * viewspaceDepth;
	ret.z = -viewspaceDepth;
	return ret;
}

//// From XeGTAO
float ScreenSpaceToViewSpaceDepth(const float screenDepth)
{
	float depthLinearizeMul = u_Camera.DepthUnpackConsts.x;
	float depthLinearizeAdd = u_Camera.DepthUnpackConsts.y;
	// Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"
	return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

void InitialAdvanceRay(vec3 origin, vec3 direction, vec3 invDirection, vec2 currentMipResolution, vec2 currentMipResolutionInv, vec2 floorOffset, vec2 uvOffset, out vec3 position, out float currentT)
{
	vec2 currentMipPosition = currentMipResolution * origin.xy;

	// Intersect ray with the half box that is pointing away from the ray origin.
	vec2 xyPlane = floor(currentMipPosition) + floorOffset;
	xyPlane = xyPlane * currentMipResolutionInv + uvOffset;

	// o + d * t = p' => t = (p' - o) / d
	vec2 t = xyPlane * invDirection.xy - origin.xy * invDirection.xy;
	currentT = min(t.x, t.y);
	position = origin + currentT * direction;
}

void SecondAdvanceRay(vec3 origin, vec3 direction, vec3 invDirection, vec2 currentMipPosition, vec2 currentMipResolutionInv, vec2 floorOffset, vec2 uvOffset, inout vec3 position, inout float currentT)
{
	// Create boundary planes
	vec2 xyPlane = floor(currentMipPosition) + floorOffset;
	xyPlane = xyPlane * currentMipResolutionInv + uvOffset;

	// o + d * t = p' => t = (p' - o) / d
	vec2 t = xyPlane * invDirection.xy - origin.xy * invDirection.xy;
	currentT = min(t.x, t.y);
	position = origin + currentT * direction;
}

bool AdvanceRay(vec3 origin, vec3 direction, vec3 invDirection, vec2 currentMipPosition, vec2 currentMipResolutionInv, vec2 floorOffset, vec2 uvOffset, float surfaceZ, inout vec3 position, inout float currentT)
{
	// Create boundary planes
	vec2 xyPlane = floor(currentMipPosition) + floorOffset;
	xyPlane = xyPlane * currentMipResolutionInv + uvOffset;
	vec3 boundaryPlanes = vec3(xyPlane, surfaceZ);

	// Intersect ray with the half box that is pointing away from the ray origin.
	// o + d * t = p' => t = (p' - o) / d
	vec3 t = boundaryPlanes * invDirection - origin * invDirection;

	// Prevent using z plane when shooting out of the depth buffer.
	#ifdef INVERTED_DEPTH_RANGE
	t.z = direction.z < 0 ? t.z : FLOAT_MAX;
	#else
	t.z = direction.z > 0 ? t.z : FLOAT_MAX;
	#endif

		// Choose nearest intersection with a boundary.
	float tMin = min(min(t.x, t.y), t.z);

	#ifdef INVERTED_DEPTH_RANGE
		// Larger z means closer to the camera.
	bool aboveSurface = surfaceZ < position.z;
	#else
		// Smaller z means closer to the camera.
	bool aboveSurface = surfaceZ > position.z;
	#endif

	// Decide whether we are able to advance the ray until we hit the xy boundaries or if we had to clamp it at the surface.
	// We use the asuint comparison to avoid NaN / Inf logic, also we actually care about bitwise equality here to see if t_min is the t.z we fed into the min3 above.
	bool skippedTile = floatBitsToUint(tMin) != floatBitsToUint(t.z) && aboveSurface;

	// Make sure to only advance the ray if we're still above the surface.
	currentT = aboveSurface ? tMin : currentT;

	// Advance ray
	position = origin + currentT * direction;

	return skippedTile;
}

// Requires origin and direction of the ray to be in screen space [0, 1] x [0, 1]
vec3 HierarchicalRaymarch(vec3 origin, vec3 direction, vec2 screenSize, int mostDetailedMip, uint minTraversalOccupancy, uint maxTraversalIntersections, out uint iterations, out bool validHit)
{
	const vec3 invDirection = direction != vec3(0) ? 1.0 / direction : vec3(FLOAT_MAX);

	// Start on mip with highest detail.
	int currentMip = mostDetailedMip;

	// Could recompute these every iteration, but it's faster to hoist them out and update them.
	vec2 currentMipResolution = GetDepthMipResolution(currentMip);
	vec2 currentMipResolutionInv = 1 / currentMipResolution;

	// Offset to the bounding boxes uv space to intersect the ray with the center of the next pixel.
	// This means we ever so slightly over shoot into the next region. 
	vec2 uvOffset = 0.005 * exp2(mostDetailedMip) / screenSize;
	uvOffset.x = (direction.x < 0.0 ? -uvOffset.x : uvOffset.x);
	uvOffset.y = (direction.y < 0.0 ? -uvOffset.y : uvOffset.y);

	// Offset applied depending on current mip resolution to move the boundary to the left/right upper/lower border depending on ray direction.
	vec2 floorOffset;
	floorOffset.x = (direction.x < 0.0 ? 0.0 : 1.0);
	floorOffset.y = (direction.y < 0.0 ? 0.0 : 1.0);

	float currentT;
	vec3 position;
	// Initially advance ray to avoid immediate self intersections.
	InitialAdvanceRay(origin, direction, invDirection, currentMipResolution, currentMipResolutionInv, floorOffset, uvOffset, position, currentT);
	vec2 currentMipPosition = currentMipResolution * position.xy;

	// This is a way to prevent artifacts on bumpy surfaces (rough normals).
	SecondAdvanceRay(origin, direction, invDirection, currentMipPosition, currentMipResolutionInv, floorOffset, uvOffset, position, currentT);

	iterations = 0;
	while (iterations < maxTraversalIntersections && currentMip >= mostDetailedMip)
	{
		currentMipPosition = currentMipResolution * position.xy;
		float surfaceZ = FetchDepth(ivec2(currentMipPosition), currentMip);
		bool skippedTile = AdvanceRay(origin, direction, invDirection, currentMipPosition, currentMipResolutionInv, floorOffset, uvOffset, surfaceZ, position, currentT);
		currentMip += skippedTile ? 1 : -1;
		currentMipResolution *= skippedTile ? 0.5 : 2;
		currentMipResolutionInv *= skippedTile ? 2 : 0.5;
		++iterations;
	}

	validHit = (iterations < maxTraversalIntersections);

	return position;
}

float ValidateHit(vec3 origin, vec3 ray, vec3 originPosVS, vec3 rayDirVS, float roughness)
{
	// Reject hits outside the view frustum
	if (any(lessThan(ray.xy, vec2(0.0))) || any(greaterThan(ray.xy, vec2(1.0))))
		return 0;
	
	// Reject the ray if we didnt advance the ray significantly to avoid immediate self reflection
	vec2 manhattanDist = abs(ray.xy - origin.xy);
	if (all(lessThan(manhattanDist, u_ScreenData.InvHalfResolution)))
		return 0;

	// Don't lookup radiance from the background.
	float surfaceZ = FetchDepth(ivec2(GetDepthMipResolution(0) * ray.xy), 0).x;
#ifdef INVERTED_DEPTH_RANGE
	if (surfaceZ == 0.0)
#else
	if (surfaceZ == 1.0)
#endif
		return 0;

	float surfaceDepthVS = ScreenSpaceToViewSpaceDepth(surfaceZ);
	vec3 surfacePosVS = ComputeViewspacePosition(ray.xy, surfaceDepthVS);

	float hitDepthVS = ScreenSpaceToViewSpaceDepth(ray.z);
	vec3 hitPositionVS = ComputeViewspacePosition(ray.xy, hitDepthVS);

	float distanceTravelled = length(surfacePosVS - hitPositionVS);

	// We accept all hits that are within a reasonable minimum distance below the surface.
	// Add constant in linear space to avoid growing of the reflections toward the reflected objects.
	// Roughness depth tolerance allows rough surfaces to be more depth tolerant, meaning with less failing rays.
	float confidence = 1.0f - smoothstep(0.0f, u_SSRInfo.DepthTolerance + mix(0.0f, u_SSRInfo.RoughnessDepthTolerance, roughness), distanceTravelled);

	// Fade out hits near the screen borders
	vec2 viewportSize = u_ScreenData.FullResolution;
	vec2 fov = u_SSRInfo.FadeIn * vec2(viewportSize.y / viewportSize.x, 1.0f);
	vec2 border = smoothstep(vec2(0.0f), fov, ray.xy) * (1.0f - smoothstep(1.0f - fov, vec2(1.0f), ray.xy));
	float vignette = border.x * border.y;

	// Fade rays pointing towards camera
	float fadeOnMirror = clamp(max(dot(originPosVS, rayDirVS), 0.0f) + u_SSRInfo.FacingReflectionsFading, 0.0f, 1.0f);
	return confidence * fadeOnMirror * vignette * (1.0f - roughness);
}

vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 base = ivec2(gl_GlobalInvocationID);
	const vec2 uv = u_SSRInfo.HalfRes ? u_ScreenData.InvHalfResolution * (base + 0.25) : u_ScreenData.InvFullResolution * (base + 0.50);
	const ivec2 baseDepthResolution = GetDepthMipResolution(BASE_LOD);
	const float depth = FetchDepth(ivec2(baseDepthResolution * uv), 0).r;

	const vec4 albedo = texture(u_InputColor, uv);
	const vec2 metalnessRoughness = texture(u_MetalnessRoughness, uv).xy;
	const vec3 normalVS = texture(u_Normal, uv).xyz;
	const float depthVS = ScreenSpaceToViewSpaceDepth(depth);
	const vec3 positionVS = ComputeViewspacePosition(uv, depthVS);
	const vec3 toPositionVS = normalize(positionVS.xyz);
	const vec3 reflectVS = reflect(toPositionVS, normalVS);
	const vec4 positionPrimeSS4 = u_Camera.ProjectionMatrix * vec4(positionVS.xyz + reflectVS, 1.0f);
	vec3 positionPrimeSS = (positionPrimeSS4.xyz / positionPrimeSS4.w);
	positionPrimeSS.xy = positionPrimeSS.xy * 0.5f + 0.5f;
	const vec3 positionSS = vec3(uv, depth);
	const vec3 reflectSS = vec3(positionPrimeSS - positionSS);

	uint iterations;
	bool validHit = false;
	vec3 ray = HierarchicalRaymarch(positionSS, reflectSS, baseDepthResolution, BASE_LOD, 4, u_SSRInfo.MaxSteps, iterations, validHit);

	vec4 totalColor;
	if (u_SSRInfo.EnableConeTracing)
		totalColor = ConeTracing(metalnessRoughness.g, positionSS.xy, ray.xy);
	else
		totalColor = texelFetch(u_InputColor, ivec2(ray.xy * textureSize(u_InputColor, 0)), 0);

	vec3 F0 = mix(vec3(0.04), albedo.rgb, metalnessRoughness.r);

	totalColor.rgb *= FresnelSchlickRoughness(F0, max(dot(normalVS, toPositionVS), 0.0), metalnessRoughness.g);

	float confidence = validHit ? ValidateHit(positionSS, ray, toPositionVS, reflectVS, metalnessRoughness.g) : 0;

	// From CesiumJS https://github.com/CesiumGS/cesium
	totalColor *= max(1.f, dot(totalColor.rgb, vec3(0.2125, 0.7154, 0.0721) * u_SSRInfo.LuminanceFactor)) * u_SSRInfo.Brightness;

	// Reflection occlusion
	float ao = 1.0f;

#if GTAO_REFLECTION_OCCLUSION
	#if __ZONG_GTAO_COMPUTE_BENT_NORMALS
		ao = (texture(u_GTAOTex, positionSS.xy).x >> 24) / 255.f;
	#else
		ao = texture(u_GTAOTex, positionSS.xy).x / 255.f;
	#endif
		ao = min(ao * XE_GTAO_OCCLUSION_TERM_SCALE, 1.0f);
#endif

#if HBAO_REFLECTION_OCCLUSION
		ao *= texture(u_HBAOTex, positionSS.xy).x;
#endif
	float roughnessFactor = mix(clamp(0.0, 1.0, metalnessRoughness.r + ao), 0.0625f, pow(metalnessRoughness.g, 4));

	// Confidence can be negtive. If it is, it will leave unwanted reflections.
	imageStore(outColor, ivec2(base), vec4(roughnessFactor * totalColor.rgb, max(confidence, 0.0)));
}


