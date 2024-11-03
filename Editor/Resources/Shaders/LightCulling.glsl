// ---------------------------------------
// -- Engine light culling shader --
// ---------------------------------------
// 
// References:
// - SIGGRAPH 2011 - Rendering in Battlefield 3
// - Implementation mostly adapted from https://github.com/bcrusco/Forward-Plus-Renderer
//
#version 450 core
#pragma stage : comp
#include <Buffers.glslh>

layout(set = 1, binding = 0) uniform sampler2D u_DepthMap;

#define TILE_SIZE 16
#define MAX_LIGHT_COUNT 1024

struct Sphere
{
	vec3 c;	 // Center point.
	float r;	// Radius.
};

bool TestFrustumSides(vec3 c, float r, vec3 plane0, vec3 plane1, vec3 plane2, vec3 plane3)
{
	bool intersectingOrInside0 = dot(c, plane0) < r;
	bool intersectingOrInside1 = dot(c, plane1) < r;
	bool intersectingOrInside2 = dot(c, plane2) < r;
	bool intersectingOrInside3 = dot(c, plane3) < r;

	return (intersectingOrInside0 && intersectingOrInside1 &&
		intersectingOrInside2 && intersectingOrInside3);
}

// From XeGTAO
float ScreenSpaceToViewSpaceDepth(const float screenDepth)
{
	float depthLinearizeMul = u_Camera.DepthUnpackConsts.x;
	float depthLinearizeAdd = u_Camera.DepthUnpackConsts.y;
	// Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"
	return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}
// Shared values between all the threads in the group
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visiblePointLightCount;
shared uint visibleSpotLightCount;
shared vec4 frustumPlanes[6];

// Shared local storage for visible indices, will be written out to the global buffer at the end
shared int visiblePointLightIndices[MAX_LIGHT_COUNT];
shared int visibleSpotLightIndices[MAX_LIGHT_COUNT];

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
void main()
{
    ivec2 location = ivec2(gl_GlobalInvocationID.xy);
    ivec2 itemID = ivec2(gl_LocalInvocationID.xy);
    ivec2 tileID = ivec2(gl_WorkGroupID.xy);
    ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
    uint index = tileID.y * tileNumber.x + tileID.x;

    // Initialize shared global values for depth and light count
    if (gl_LocalInvocationIndex == 0)
    {
		minDepthInt = 0xFFFFFFFF;
		maxDepthInt = 0;
		visiblePointLightCount = 0;
		visibleSpotLightCount = 0;
    }

    barrier();

    // Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
    vec2 tc = vec2(location) / u_ScreenData.FullResolution;
    float linearDepth = ScreenSpaceToViewSpaceDepth(textureLod(u_DepthMap, tc, 0).r);

    // Convert depth to uint so we can do atomic min and max comparisons between the threads
    uint depthInt = floatBitsToUint(linearDepth);
    atomicMin(minDepthInt, depthInt);
    atomicMax(maxDepthInt, depthInt);

    barrier();

    // Step 2: One thread should calculate the frustum planes to be used for this tile
    if (gl_LocalInvocationIndex == 0)
    {
		// Convert the min and max across the entire tile back to float
		float minDepth = uintBitsToFloat(minDepthInt);
		float maxDepth = uintBitsToFloat(maxDepthInt);

		// Steps based on tile sale
		vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
		vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

		// Set up starting values for planes using steps and min and max z values
		frustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x); // Left
		frustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x); // Right
		frustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y); // Bottom
		frustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y); // Top
		frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth); // Near
		frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth); // Far

		// Transform the first four planes
		for (uint i = 0; i < 4; i++)
		{
		    frustumPlanes[i] *= u_Camera.ViewProjectionMatrix;
		    frustumPlanes[i] /= length(frustumPlanes[i].xyz);
		}

		// Transform the depth planes
		frustumPlanes[4] *= u_Camera.ViewMatrix;
		frustumPlanes[4] /= length(frustumPlanes[4].xyz);
		frustumPlanes[5] *= u_Camera.ViewMatrix;
		frustumPlanes[5] /= length(frustumPlanes[5].xyz);
    }

    barrier();

    // Step 3: Cull lights.
    // Parallelize the threads against the lights now.
    // Can handle 256 simultaniously. Anymore lights than that and additional passes are performed
    const uint threadCount = TILE_SIZE * TILE_SIZE;
    uint passCount = (u_PointLights.LightCount + threadCount - 1) / threadCount;
    for (uint i = 0; i < passCount; i++)
    {
		// Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= u_PointLights.LightCount)
		    break;

		vec4 position = vec4(u_PointLights.Lights[lightIndex].Position, 1.0f);
		float radius = u_PointLights.Lights[lightIndex].Radius;
		radius += radius * 0.3f;

		// Check if light radius is in frustum
		float distance = 0.0;
		for (uint j = 0; j < 6; j++)
		{
		    distance = dot(position, frustumPlanes[j]) + radius;
		    if (distance <= 0.0) // No intersection
				break;
		}

		// If greater than zero, then it is a visible light
		if (distance > 0.0)
		{
		    // Add index to the shared array of visible indices
		    uint offset = atomicAdd(visiblePointLightCount, 1);
		    visiblePointLightIndices[offset] = int(lightIndex);
		}
    }

	passCount = (u_SpotLights.LightCount + threadCount - 1) / threadCount;
	for (uint i = 0; i < passCount; i++)
	{
		// Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= u_SpotLights.LightCount)
			break;

		SpotLight light = u_SpotLights.Lights[lightIndex];
		float radius = light.Range;
		// Check if light radius is in frustum
		float distance = 0.0;
		for (uint j = 0; j < 6; j++)
		{
			distance = dot(vec4(light.Position - light.Direction * (light.Range * 0.7), 1.0), frustumPlanes[j]) + radius * 1.3;
			if (distance < 0.0) // No intersection
				break;
		}

		// If greater than zero, then it is a visible light
		if (distance > 0.0)
		{
			// Add index to the shared array of visible indices
			uint offset = atomicAdd(visibleSpotLightCount, 1);
			visibleSpotLightIndices[offset] = int(lightIndex);
		} 
		
	}

    barrier();

    // One thread should fill the global light buffer
    if (gl_LocalInvocationIndex == 0)
    {
		const uint offset = index * MAX_LIGHT_COUNT; // Determine position in global buffer
		for (uint i = 0; i < visiblePointLightCount; i++) 
		{
			s_VisiblePointLightIndicesBuffer.Indices[offset + i] = visiblePointLightIndices[i];
		}

		for (uint i = 0; i < visibleSpotLightCount; i++) {
			s_VisibleSpotLightIndicesBuffer.Indices[offset + i] = visibleSpotLightIndices[i];
		}

		if (visiblePointLightCount != MAX_LIGHT_COUNT)
		{
		    // Unless we have totally filled the entire array, mark it's end with -1
		    // Final shader step will use this to determine where to stop (without having to pass the light count)
			s_VisiblePointLightIndicesBuffer.Indices[offset + visiblePointLightCount] = -1;
		}

		if (visibleSpotLightCount != MAX_LIGHT_COUNT)
		{
			// Unless we have totally filled the entire array, mark it's end with -1
			// Final shader step will use this to determine where to stop (without having to pass the light count)
			s_VisibleSpotLightIndicesBuffer.Indices[offset + visibleSpotLightCount] = -1;
		}
    }
}