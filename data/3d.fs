#version 330 core
flat in vec3 oNormal;
in vec2 oTexture;
in vec4 oShadowCoord;

out vec4 colour;

uniform sampler2D sampler0;
uniform sampler2D sampler1;
//uniform vec3 diffuse;

void main(void) {
  vec3 diffuse = vec3(1.0f, 1.0f, 1.0f);
  float visibility = 1.0f;
  vec4 sampler1Coord = texture(sampler1, oShadowCoord.xy);
  if (sampler1Coord.z < oShadowCoord.z - 0.005f)
    visibility = 0.5f;

  colour = vec4(diffuse*visibility, 1.0f);//texture(sampler, oTexture);
}
