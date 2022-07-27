#version 440 core
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in vec3 vertpos[];
in vec3 normal[];
in vec2 texpos[];

out vec3 vertposG;
out vec3 normalG;
out vec2 texposG;

uniform float exploded;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void
main(void)
{
  vec4 triangleNormal = vec4(((normal[0] + normal[1] + normal[2]) / 3.0), 1.0);

  for (int i = 0; i < 3; ++i) {
    gl_Position = projection * (gl_in[i].gl_Position + normalize(triangleNormal) * (exploded * 2.5));
    vertposG = vertpos[i];
    normalG = normal[i];
    texposG = texpos[i];
    EmitVertex();
  }
  EndPrimitive();
}
