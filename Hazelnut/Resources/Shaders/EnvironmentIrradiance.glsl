#version 450 core
#pragma stage : comp

#include <EnvironmentMapping.glslh>

// Computes diffuse irradiance cubemap convolution for image-based lighting.
// Uses quasi Monte Carlo sampling with Hammersley sequence.

layout(binding = 0, rgba32f) restrict writeonly uniform imageCube o_IrradianceMap;
layout(binding = 1) uniform samplerCube u_RadianceMap;

layout(push_constant) uniform Uniforms
{
	uint Samples;
} u_Uniforms;

layout(local_size_x=32, local_size_y=32, local_size_z=1) in;
void main()
{
	vec3 N = GetCubeMapTexCoord(vec2(imageSize(o_IrradianceMap)));
	
	vec3 S, T;
	ComputeBasisVectors(N, S, T);

	uint samples = 64 * u_Uniforms.Samples;

	// Monte Carlo integration of hemispherical irradiance.
	// As a small optimization this also includes Lambertian BRDF assuming perfectly white surface (albedo of 1.0)
	// so we don't need to normalize in PBR fragment shader (so technically it encodes exitant radiance rather than irradiance).
	vec3 irradiance = vec3(0);
	for(uint i = 0; i < samples; i++)
	{
		vec2 u  = SampleHammersley(i, samples);
		vec3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N, S, T);
		float cosTheta = max(0.0, dot(Li, N));

		// PIs here cancel out because of division by pdf.
		irradiance += 2.0 * textureLod(u_RadianceMap, Li, 0).rgb * cosTheta;
	}
	irradiance /= vec3(samples);

	imageStore(o_IrradianceMap, ivec3(gl_GlobalInvocationID), vec4(irradiance, 1.0));
}
