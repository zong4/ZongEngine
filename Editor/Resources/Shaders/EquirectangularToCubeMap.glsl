// Converts equirectangular (lat-long) projection texture into a cubemap
#version 450 core
#pragma stage : comp
#include <EnvironmentMapping.glslh>



layout(binding = 0, rgba16f) restrict writeonly uniform imageCube o_CubeMap;
layout(binding = 1) uniform sampler2D u_EquirectangularTex;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
	vec3 cubeTC = GetCubeMapTexCoord(vec2(imageSize(o_CubeMap)));

    // Calculate sampling coords for equirectangular texture
	// https://en.wikipedia.org/wiki/Spherical_coordinate_system#Cartesian_coordinates
	float phi = atan(cubeTC.z, cubeTC.x);
	float theta = acos(cubeTC.y);
    vec2 uv = vec2(phi / (2.0 * PI) + 0.5, theta / PI);

	vec4 color = texture(u_EquirectangularTex, uv);
	imageStore(o_CubeMap, ivec3(gl_GlobalInvocationID), color);
}
