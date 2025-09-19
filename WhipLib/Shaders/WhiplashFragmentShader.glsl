#version 430

in vec2 vtfTexCoords;
in vec3 vtfNormalModel;
in vec3 vtfNormalWorld;
in vec3 vtfVertexPositionWorld;

out vec4 daColor;

uniform sampler2D textureSlot;
uniform vec3 eyePositionWorld;
uniform int wireframe;

void main()
{
  vec3 eyeVectorWorld = normalize(eyePositionWorld - vtfVertexPositionWorld);
  float s = dot(vtfNormalWorld, eyeVectorWorld);
  vec4 textureColor = texture(textureSlot, vtfTexCoords);
  if (wireframe == 1) {
    daColor = vec4(1.0 - textureColor.r, 1.0 - textureColor.g, 1.0 - textureColor.b, 1.0);
  } else {
    daColor = textureColor;
  }
}