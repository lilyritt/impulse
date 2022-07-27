#version 440 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

out vec3 vertpos;
out vec3 normal;
out vec2 texpos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void
main(void)
{
  normal = mat3(transpose(inverse(model))) * norm;
  vertpos = vec3(model * vec4(pos, 1.0));
  texpos = tex;
  // texpos = vec3(model * vec4(tex, 0.0, 1.0));
  gl_Position = view  * model * vec4(pos, 1.0);
}
