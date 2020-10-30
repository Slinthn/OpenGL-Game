#version 330 core
layout (location = 0) in vec3 vPosition;

uniform mat4 transform;

uniform mat4 shadowProjection;
uniform mat4 shadowTransform; // TODO(slin): Does this need to be inversed? yeah, right? it's basically a camera?

void main(void) {
  gl_Position = shadowProjection*inverse(shadowTransform)*transform*vec4(vPosition, 1.0f);
}
