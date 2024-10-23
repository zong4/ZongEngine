#version 430 core
#pragma stage:vert

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 vs_TexCoords;

void main()
{
    vs_TexCoords = a_TexCoord;
    gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#version 450 core
#pragma stage:frag

layout(set = 1, binding = 0) uniform sampler2D u_SSR;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 vs_TexCoords;

void main()
{
    outColor = texture(u_SSR, vs_TexCoords); 
}
