#version 450 core
#pragma stage : vert

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) out OutputBlock Output;

void main()
{
	vec4 position = vec4(a_Position.xy, 0.0, 1.0);
	Output.TexCoord = a_TexCoord;
	gl_Position = position;
}

#version 450 core
#pragma stage : frag

#include <Buffers.glslh>

layout(location = 0) out vec4 o_Color;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) in OutputBlock Input;

layout (set = 1, binding = 0) uniform sampler2D u_ViewNormalsTexture;
layout (set = 1, binding = 1) uniform sampler2D u_DepthTexture;

float LinearizeDepth(const float screenDepth)
{
	float depthLinearizeMul = u_Camera.DepthUnpackConsts.x;
	float depthLinearizeAdd = u_Camera.DepthUnpackConsts.y;
	return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

float checkSame(vec4 center, float centerDepth, vec4 samplef, float sampleDepth, vec2 sensitivity)
{
    vec2 centerNormal = center.xy;
    vec2 sampleNormal = samplef.xy;
    
    vec2 diffNormal = abs(centerNormal - sampleNormal) * sensitivity.x;
    bool isSameNormal = (diffNormal.x + diffNormal.y) < 0.1;
    float diffDepth = abs(centerDepth - sampleDepth) * sensitivity.y;
    bool isSameDepth = diffDepth < 0.1;
    
    return (isSameNormal && isSameDepth) ? 1.0 : 0.0;
    return (isSameNormal) ? 1.0 : 0.0;
}

void main()
{
    ivec2 texSize = textureSize(u_ViewNormalsTexture, 0);
    vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));

    vec2 sensitivity = (vec2(0.3, 1.5) * fTexSize.y / 800.0);
    vec2 singleTexel = vec2(1.0) / fTexSize;

    vec4 sample1 = texture(u_ViewNormalsTexture, Input.TexCoord + singleTexel);
    vec4 sample2 = texture(u_ViewNormalsTexture, Input.TexCoord + -singleTexel);
    vec4 sample3 = texture(u_ViewNormalsTexture, Input.TexCoord + vec2(-singleTexel.x, singleTexel.y));
    vec4 sample4 = texture(u_ViewNormalsTexture, Input.TexCoord + vec2(singleTexel.x, -singleTexel.y));

    float sampleDepth1 = LinearizeDepth(texture(u_DepthTexture, Input.TexCoord + singleTexel).r) / 10.0;
    float sampleDepth2 = LinearizeDepth(texture(u_DepthTexture, Input.TexCoord + -singleTexel).r)/ 10.0;
    float sampleDepth3 = LinearizeDepth(texture(u_DepthTexture, Input.TexCoord + vec2(-singleTexel.x, singleTexel.y)).r)/ 10.0;
    float sampleDepth4 = LinearizeDepth(texture(u_DepthTexture, Input.TexCoord + vec2(singleTexel.x, -singleTexel.y)).r)/ 10.0;
    
    float edge = checkSame(sample1, sampleDepth1, sample2, sampleDepth2, sensitivity) * checkSame(sample3, sampleDepth3, sample4, sampleDepth4, sensitivity);

	//float rawDepth = texture(u_DepthTexture, Input.TexCoord).r;
	//float depth = LinearizeDepth(rawDepth);

	o_Color = vec4(vec3(edge), 1.0);
}