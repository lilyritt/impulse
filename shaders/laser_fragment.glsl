#version 440 core

out vec4 color;

uniform float life;

void
main(void)
{
  // #588189
  vec3 base_color = vec3(0.345, 0.506, 0.537) * 0.75;
  color = mix(vec4(1.0), vec4(base_color, 1.0), life);
  /*
  if (life > 0.75) {
    color = vec4(1.0);
  } else if (life < 0.25) {
    color = vec4(base_color * 0.5, 1.0);
  } else {
    color = vec4(base_color, 1.0);
  }
  */
}
