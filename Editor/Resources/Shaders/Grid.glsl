// Grid Shader

#version 450 core
#pragma stage : vert
#include <Buffers.glslh>

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 v_TexCoord;

layout(push_constant) uniform Transform
{
	mat4 Transform;
} u_Renderer;

void main()
{
    vec4 position = u_Camera.ViewProjectionMatrix * u_Renderer.Transform * vec4(a_Position, 1.0);
    gl_Position = position;

    v_TexCoord = a_TexCoord;
}

#version 450 core
#pragma stage : frag

layout(location = 0) out vec4 color;

layout(push_constant) uniform Settings
{
	layout(offset = 64) float Scale;
	float Size;
} u_Settings;

layout(location = 0) in vec2 v_TexCoord;

float grid(vec2 st, float res)
{
    vec2 grid = fract(st);
    return step(res, grid.x) * step(res, grid.y);
}

void main()
{
	float x = grid(v_TexCoord * u_Settings.Scale, u_Settings.Size);
	color = vec4(vec3(0.2), 0.5) * (1.0 - x);
	
	if (color.a == 0.0)
		discard;
}
