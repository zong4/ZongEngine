#version 430 core
#pragma stage : vert

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 o_TexCoord;
void main()
{
    o_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#version 450 core
#pragma stage : frag
#include <GTAO.slh>

#define ENABLED_GTAO (__ZONG_AO_METHOD & ZONG_AO_METHOD_GTAO)
#define ENABLED_HBAO (__ZONG_AO_METHOD & ZONG_AO_METHOD_HBAO)

#if ENABLED_GTAO
layout(set = 1, binding = 0) uniform usampler2D u_GTAOTex;
#endif

#if ENABLED_HBAO
layout(binding = 1) uniform sampler2D u_HBAOTex;
#endif

layout(location = 0) out vec4 o_Occlusion;
layout(location = 0) in vec2 vs_TexCoord;



void main()
{
    float occlusion = 1.0f;
#if ENABLED_GTAO
    #if __ZONG_GTAO_COMPUTE_BENT_NORMALS
        float ao = (texture(u_GTAOTex, vs_TexCoord).x >> 24) / 255.f;
    #else
        float ao = texture(u_GTAOTex, vs_TexCoord).x / 255.f;
    #endif
        occlusion = min(ao * XE_GTAO_OCCLUSION_TERM_SCALE, 1.0f);
#endif
#if ENABLED_HBAO
    occlusion *= texture(u_HBAOTex, vs_TexCoord).x;
#endif

    o_Occlusion = occlusion.xxxx;
}

