#version 440 core

out vec4 color;

uniform uint pattern;
uniform float life;

void
main(void)
{
  // #588189
  vec3 base_color = vec3(0.345, 0.506, 0.537) * 0.75;
  color = mix(vec4(1.0), vec4(base_color, 1.0), life);

  if (pattern == 1) color = vec4(1.0, 0.0, 0.0, 1.0);
  if (pattern == 2) color = vec4(0.0, 1.0, 0.0, 1.0);
  if (pattern == 3) color = vec4(0.0, 0.0, 1.0, 1.0);

  /* Rainbow pride */
  if (pattern == 4) {
    if (life < 1.0 / 6.0)
      color = vec4(228.0 / 255.0, 3.0 / 255.0, 3.0 / 255.0, 1.0);
    else if (life < 2.0 / 6.0)
      color = vec4(1.0, 140.0 / 255.0, 0.0, 1.0);
    else if (life < 3.0 / 6.0)
      color = vec4(1.0, 237.0 / 255.0, 0.0, 1.0);
    else if (life < 4.0 / 6.0)
      color = vec4(0.0, 128.0 / 255.0, 38.0 / 255.0, 1.0);
    else if (life < 5.0 / 6.0)
      color = vec4(0.0, 77.0 / 255.0, 1.0, 1.0);
    else
      color = vec4(117.0 / 255.0, 7.0 / 255.0, 135.0 / 255.0, 1.0);
  }

  /* Trans pride */
  if (pattern == 5) {
    if (life < 0.2 || life > 0.8)
      color = vec4(91.0 / 255.0, 206.0 / 255.0, 250.0 / 255.0, 1.0);
    else if (life < 0.4 || life > 0.6)
      color = vec4(245.0 / 255.0, 169.0 / 255.0, 184.0 / 255.0, 1.0);
    else
      color = vec4(1.0);
  }
}
