#version 330 core

out float fragmentDepth;

void main(void) {
  fragmentDepth = gl_FragCoord.z;
}
