#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexture;
layout (location = 2) in vec3 vNormal;

out vec2 oTexture;
flat out vec3 oNormal;
out vec3 oToLight;
out vec4 oShadowCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 transform;

uniform mat4 shadowProjection;
uniform mat4 shadowTransform;

void main(void) {
  vec4 worldPosition = transform*vec4(vPosition, 1.0f);
  gl_Position = projection*inverse(view)*worldPosition;
  oTexture = vTexture;
  oNormal = (transform*vec4(vNormal, 0.0f)).xyz;

  oShadowCoord = shadowProjection*inverse(shadowTransform)*vec4(worldPosition.xyz, 1.0f);
  oShadowCoord *= 0.5f;
  oShadowCoord += 0.5f;
}
