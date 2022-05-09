#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragTexCoord;

layout (binding = 0) uniform UBO 
{
  mat4 projectionMatrix;
  vec3 directionToLight;
  mat4 view;
} ubo;

layout(push_constant) uniform Push {
  mat4 modelMatrix; // projection * view * model
  mat4 normalMatrix;
  vec3 colorPush;
} push;

void main()
{
    gl_Position = ubo.projectionMatrix * ubo.view * vec4(position.xyz, 1.0);
    fragTexCoord = position;
    fragTexCoord.xy *= -1.0;
}