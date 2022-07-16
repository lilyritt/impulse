#version 440 core

in vec3 vertpos;
in vec3 normal;
in vec2 texpos;

out vec4 color;

uniform float health;
uniform vec3 lightPos;
uniform sampler2D text;

void
main(void)
{
  // #588189
  // vec3 base_color = vec3(122.0 / 255.0, 83.0 / 255.0, 24.0 / 255.0);
  vec4 objectColor = texture(text, texpos);

  // #fbfcdb
  vec4 lightBase = vec4(252.0 / 255.0, 250.0 / 255.0, 249.0 / 255, 1.0);
  vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
  vec4 lightColor = mix(red, lightBase, health);

  // vec3 lightPos = vec3(7.0, 7.0, 7.0);
  vec3 viewPos = vec3(0.0);

  float ambientStrength = 0.1;
  vec4 ambient = ambientStrength * lightColor;

  vec3 norm = normalize(normal);
  vec3 lightDir = normalize(lightPos - vertpos);
  float diff = max(dot(norm, lightDir), 0.0);

  float specularStrength = 0.5;
  vec3 viewDir = normalize(viewPos - vertpos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64) * specularStrength;

  vec4 light = (ambient + diff + spec) * lightColor;

  color = light * objectColor;
}
