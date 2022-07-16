#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

void framebuffer_size_callback(GLFWwindow *, i32, i32);
char *read_to_string(const char *);
u32 load_shaders(const char *, const char *);
void setup(void);
void add_asteroid(void);
void shoot_laser(f64, f32);
void update(f64);
void clamp_boundry(struct point *);
void draw(void);
f32 to_rads(f32);
f32 aspect_ratio(void);
void update_scoreboard(void);
bool asteroid_collision(struct asteroid *, f32);

struct point {
  f32 x;
  f32 y;
};

struct asteroid {
  struct point pos;
  struct point vel;
  u8 health;
};

struct laser {
  struct point pos;
  struct point vel;
  f64 timestamp;
};

struct vao {
  u32 ship;
  u32 laser;
  u32 asteroid;
};

struct shaders {
  u32 ship;
  u32 laser;
  u32 asteroid;
};

enum movement {
  MOVEMENT_NONE,
  MOVEMENT_LEFT,
  MOVEMENT_RIGHT,
};

struct state {
  i32 width;
  i32 height;
  GLFWwindow *window;
  struct shaders shaders;
  struct vao vao;

  struct point pos;
  f32 dir;
  enum movement movement;
  struct point vel;
  bool boost;
  u8 health;
  u32 score;
  f64 last_laser; /* for laser cooldown */

  u32 asteroid_indices;
  u32 asteroid_texture;
  f32 fov;

  std::vector<struct asteroid> asteroids;
  std::vector<struct laser> lasers;
};

static const u8 MAX_ASTEROID_HEALTH = 3;

static struct state *state;

int
main(int, char **)
{
  setup();

  f64 dt, time = glfwGetTime();
  while (!glfwWindowShouldClose(state->window)) {
    dt = glfwGetTime() - time;
    time = glfwGetTime();
    update(dt);
    draw();
    glfwSwapBuffers(state->window);
    glfwPollEvents();
    // printf("FPS: %.0f    \r", 1.0 / (glfwGetTime() - time)); fflush(stdout);
  }

  delete state;
}

void
setup(void)
{
  state = new struct state;
  state->width = 800;
  state->height = 450;
  state->health = 5;
  state->score = 0;
  state->dir = 90.0;
  state->pos.x = 0.0;
  state->pos.y = 0.0;
  state->vel.x = 0.0;
  state->vel.y = 0.0;
  state->boost = false;
  state->last_laser = 0.0;
  state->fov = 80.0;
  puts("IMPULSE");
  update_scoreboard();
  srand48(time(NULL));
  for (u8 i = 0; i < 3; ++i)
    add_asteroid();

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  atexit(glfwTerminate);


  state->window = glfwCreateWindow(state->width, state->height, "Impulse", NULL, NULL);
  if (state->window == NULL)
    errx(EXIT_FAILURE, "Failed to create GLFW window");
  glfwMakeContextCurrent(state->window);

  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    errx(EXIT_FAILURE, "Failed to initialize GLAD");

  glfwSetFramebufferSizeCallback(state->window, framebuffer_size_callback);

  /* The ship */
  f32 ship[] = {
    /* Top left */
     0.0,   1.5,   0.0,
    -0.5,   0.0,   0.0,
     0.0,   0.2,   0.3,

    /* Top right */
     0.0,   1.5,   0.0,
     0.0,   0.2,   0.3,
     0.5,   0.0,   0.0,

    /* Back */
     0.0,   0.2,   0.3,
    -0.5,   0.0,   0.0,
     0.5,   0.0,   0.0,

    /* Left booster */
     0.0,   0.0,   0.0,
    -0.5,   0.0,   0.0,
    -0.3,  -0.5,   0.1,

    /* Right booster */
     0.0,   0.0,   0.0,
     0.3,  -0.5,   0.1,
     0.5,   0.0,   0.0,
  };

  glGenVertexArrays(1, &state->vao.ship);
  glBindVertexArray(state->vao.ship);

  u32 shipvbo;
  glGenBuffers(1, &shipvbo);
  glBindBuffer(GL_ARRAY_BUFFER, shipvbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(ship), ship, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(f32), (void *) 0);
  glEnableVertexAttribArray(0);

  state->shaders.ship = load_shaders("shaders/ship_vertex.glsl", "shaders/ship_fragment.glsl");

  /* Lasers */
  f32 laser[] = {
     1.0,  1.0,  0.0,
    -1.0,  1.0,  0.0,
    -1.0, -1.0,  0.0,

     1.0,  1.0,  0.0,
    -1.0,  -1.0,  0.0,
     1.0,  -1.0,  0.0
  };
  for (u8 i = 0; i < 18; ++i)
    laser[i] *= 0.15;

  glGenVertexArrays(1, &state->vao.laser);
  glBindVertexArray(state->vao.laser);

  u32 laservbo;
  glGenBuffers(1, &laservbo);
  glBindBuffer(GL_ARRAY_BUFFER, laservbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(laser), laser, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(f32), (void *) 0);
  glEnableVertexAttribArray(0);

  state->shaders.laser = load_shaders("shaders/laser_vertex.glsl", "shaders/laser_fragment.glsl");

  /* Asteroids */
  /*
  f32 asteroid[] = {
     1.0,  1.0,  0.0,
    -1.0,  1.0,  0.0,
    -1.0, -1.0,  0.0,

     1.0,  1.0,  0.0,
    -1.0, -1.0,  0.0,
     1.0, -1.0,  0.0,
  };
  */

  Assimp::Importer importer;
  const struct aiScene *phobos = importer.ReadFile("data/phobos.glb",
                                                   aiProcessPreset_TargetRealtime_Quality);

  if (!phobos)
    errx(EXIT_FAILURE, "Failure to import asteroid model: %s", importer.GetErrorString());

  assert(phobos->mNumMeshes == 1);

  // 1 mesh, 2 mats, 1 texture, yes has normals

  const struct aiMesh *phobos_mesh = phobos->mMeshes[0];
  state->asteroid_indices  = phobos_mesh->mNumFaces * 3;

  u32 *astr_indices = new u32[state->asteroid_indices];
  for (u32 i = 0; i < phobos_mesh->mNumFaces; ++i) {
    const struct aiFace face = phobos_mesh->mFaces[i];
    assert(face.mNumIndices == 3);
    astr_indices[3*i] = face.mIndices[0];
    astr_indices[3*i+1] = face.mIndices[1];
    astr_indices[3*i+2] = face.mIndices[2];
  }

  u32 astr_vert_len = phobos_mesh->mNumFaces * 8;
  f32 *astr_vertices = new f32[astr_vert_len];
  for (u32 i = 0; i < phobos_mesh->mNumVertices; ++i) {
    const aiVector3D vert = phobos_mesh->mVertices[i];
    const aiVector3D norm = phobos_mesh->mNormals[i];
    const aiVector3D text = phobos_mesh->mTextureCoords[0][i];
    astr_vertices[i*8] = vert[0];
    astr_vertices[i*8+1] = vert[1];
    astr_vertices[i*8+2] = vert[2];
    astr_vertices[i*8+3] = norm[0];
    astr_vertices[i*8+4] = norm[1];
    astr_vertices[i*8+5] = norm[2];
    astr_vertices[i*8+6] = text[0];
    astr_vertices[i*8+7] = text[1];
  }

  const struct aiTexture *rawpngtexture = phobos->mTextures[0];
  assert(rawpngtexture->mHeight == 0);
  assert(strcmp(rawpngtexture->achFormatHint, "png") == 0);

  stbi_set_flip_vertically_on_load(true);
  i32 texwidth, texheight, texnchannels;
  u8 *texturedata = stbi_load_from_memory((u8 *) rawpngtexture->pcData,
                                          rawpngtexture->mWidth,
                                          &texwidth, &texheight, &texnchannels, 0);

  if (!texturedata)
    errx(EXIT_FAILURE, "Failure to load embedded asteroid texture data");

  glGenVertexArrays(1, &state->vao.asteroid);
  glBindVertexArray(state->vao.asteroid);

  glGenTextures(1, &state->asteroid_texture);
  glBindTexture(GL_TEXTURE_2D, state->asteroid_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwidth, texheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturedata);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(texturedata);

  u32 asteroidvbo, asteroidebo;
  glGenBuffers(1, &asteroidvbo);
  glBindBuffer(GL_ARRAY_BUFFER, asteroidvbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * astr_vert_len, astr_vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &asteroidebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asteroidebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * state->asteroid_indices, astr_indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(f32), (void *) 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, false, 8 * sizeof(f32), (void *) (3 * sizeof(f32)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, false, 8 * sizeof(f32), (void *) (6 * sizeof(f32)));
  glEnableVertexAttribArray(2);

  delete[] astr_indices;
  delete[] astr_vertices;

  state->shaders.asteroid = load_shaders("shaders/asteroid_vertex.glsl", "shaders/asteroid_fragment.glsl");

  // #9dd5e0
  // glClearColor(157.0 / 255.0, 213.0 / 255.0, 224.0 / 255.0, 1.0);

  glEnable(GL_DEPTH_TEST);
}

bool
asteroid_collision(struct asteroid *astr, f32 size)
{
  struct point center;
  center.x = state->pos.x - cosf(to_rads(state->dir)) * 0.5;
  center.y = state->pos.y + sinf(to_rads(state->dir)) * 0.5;

  return fabsf(center.x - astr->pos.x) <= size &&
         fabsf(center.y - astr->pos.y) <= size;
}

void
add_asteroid(void)
{
  struct asteroid astr;
  astr.vel.x = (drand48() - 0.5) * 5.0;
  astr.vel.y = (drand48() - 0.5) * 5.0;
  astr.health = MAX_ASTEROID_HEALTH;
  do {
    astr.pos.x = (drand48() - 0.5) * 20.0;
    astr.pos.y = (drand48() - 0.5) * 20.0;
  } while (asteroid_collision(&astr, 4.0));
  state->asteroids.push_back(astr);
}

void
shoot_laser(f64 time, f32 dir)
{
  f32 ship_influence = 0.05;
  struct laser laser;
  laser.pos.x = state->pos.x;
  laser.pos.y = state->pos.y;
  laser.vel.x = state->vel.x * ship_influence - cosf(to_rads(dir));
  laser.vel.y = state->vel.y * ship_influence + sinf(to_rads(dir));
  laser.timestamp = state->last_laser = time;
  state->lasers.push_back(laser);
}

void
update(f64 dt)
{
  f64 time = glfwGetTime();

  GLFWwindow *w = state->window;

  if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
      glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) {
    puts("\nquit");
    glfwSetWindowShouldClose(w, true);
    return;
  }

  /* Movement */
  f32 turn_speed = 250.0;
  f32 acceleration = 15.0;
  f32 max_vel = 10.0;
  if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS ||
      glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) {
    turn_speed *= 0.75;
    state->boost = true;
    state->vel.x -= cosf(to_rads(state->dir)) * acceleration * dt;
    state->vel.y += sinf(to_rads(state->dir)) * acceleration * dt;
    if (state->vel.x > max_vel)  state->vel.x = max_vel;
    if (state->vel.x < -max_vel) state->vel.x = -max_vel;
    if (state->vel.y > max_vel)  state->vel.y = max_vel;
    if (state->vel.y < -max_vel) state->vel.y = -max_vel;
  } else
    state->boost = false;

  state->movement = MOVEMENT_NONE;
  if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS ||
      glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) {
    state->dir += turn_speed * dt;
    state->movement = MOVEMENT_RIGHT;
  }
  if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS ||
      glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) {
    state->dir -= turn_speed * dt;
    state->movement = state->movement == MOVEMENT_RIGHT ? MOVEMENT_NONE : MOVEMENT_LEFT;
  }
  if (state->dir < 0.0)
    state->dir += 360.0;
  if (state->dir > 360.0)
    state->dir -= 360.0;

  state->pos.x += state->vel.x * dt;
  state->pos.y += state->vel.y * dt;
  clamp_boundry(&state->pos);

  /* Asteroid collision */
  f32 asteroid_size = 1.2;
  for (u32 i = 0; i < state->asteroids.size(); ++i) {
    struct asteroid &a = state->asteroids[i];
    if (asteroid_collision(&a, 1.9)) {
      --state->health;
      state->asteroids.erase(state->asteroids.begin() + i--);
      update_scoreboard();
      if (state->health == 0) {
        puts("\nGame over!");
        glfwSetWindowShouldClose(state->window, true);
      }
    }
  }

  /* Shooting */
  f64 cooldown = 0.15;
  if ((glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS ||
       glfwGetKey(w, GLFW_KEY_X) == GLFW_PRESS) &&
      time - state->last_laser >= cooldown) {
    if (state-> score < 10)
      shoot_laser(time, state->dir);
    else if (state-> score < 20)
      for (f32 i = -0.5; i <= 0.5; i += 1.0)
        shoot_laser(time, state->dir + i * 7.5);
    else
      for (i32 i = -1; i <= 1; ++i)
        shoot_laser(time, state->dir + i * 15.0);
  }

  f64 laser_duration = 1.0;
  f32 laser_speed = 15.0;
  for (u32 i = 0; i < state->lasers.size(); ++i) {
    struct laser &l = state->lasers[i];
    f32 first_boost = l.timestamp == time ? 2 : 1;
    l.pos.x += l.vel.x * laser_speed * first_boost * dt;
    l.pos.y += l.vel.y * laser_speed * first_boost * dt;

    clamp_boundry(&l.pos);

    /* Kill dead lasers */
    if (time - l.timestamp >= laser_duration) {
      state->lasers.erase(state->lasers.begin() + i--);
      continue;
    }

    /* Asteroid-laser collision */
    for (u32 j = 0; j < state->asteroids.size(); ++j) {
      struct asteroid &a = state->asteroids[j];
      if (fabsf(l.pos.x - a.pos.x) <= asteroid_size &&
          fabsf(l.pos.y - a.pos.y) <= asteroid_size) {
        a.health -= 1;
        state->lasers.erase(state->lasers.begin() + i--);
        if (a.health == 0) {
          ++state->score;
          state->asteroids.erase(state->asteroids.begin() + j--);
          update_scoreboard();
          continue;
        }
      }
    }
  }

  /* Move asteroids */
  f32 asteroid_speed = 1.0;
  for (u32 i = 0; i < state->asteroids.size(); ++i) {
    struct asteroid &astr = state->asteroids[i];
    astr.pos.x += astr.vel.x * asteroid_speed * dt;
    astr.pos.y += astr.vel.y * asteroid_speed * dt;
    clamp_boundry(&astr.pos);
  }

  if (state->asteroids.empty())
    for (u8 i = 0; i < 5; ++i)
      add_asteroid();


  /* Debug */
  if (glfwGetKey(w, GLFW_KEY_F1) == GLFW_PRESS) {
    state->vel.x = 0.0;
    state->vel.y = 0.0;
  }

  if (glfwGetKey(w, GLFW_KEY_F2) == GLFW_PRESS) {
    state->pos.x = 0.0;
    state->pos.y = 0.0;
  }

  if (glfwGetKey(w, GLFW_KEY_F3) == GLFW_PRESS) {
    f32 mod = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 1.0 : -1.0;
    state->fov += mod * 10.0 * dt;
  }

  // printf("dir: %.3f velocity: %.3f %.3f pos: %.3f %.3f\r", state->dir, state->vel.x, state->vel.y, state->pos.x, state->pos.y); fflush(stdout);

}

void
draw(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
  glm::mat4 projection = glm::perspective(glm::radians(state->fov), aspect_ratio(), 0.1f, 100.0f);

  /* Asteroids */
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glUseProgram(state->shaders.asteroid);
  glBindVertexArray(state->vao.asteroid);
  for (u32 i = 0; i < state->asteroids.size(); ++i) {
    struct asteroid &astr = state->asteroids[i];
    f32 scale_factor = 0.1f;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(astr.pos.x, astr.pos.y, 0.0f));
    model = glm::scale(model, glm::vec3(scale_factor));
    i32 loc_model = glGetUniformLocation(state->shaders.asteroid, "model");
    i32 loc_view = glGetUniformLocation(state->shaders.asteroid, "view");
    i32 loc_projection = glGetUniformLocation(state->shaders.asteroid, "projection");
    i32 loc_health = glGetUniformLocation(state->shaders.asteroid, "health");
    i32 loc_lightpos = glGetUniformLocation(state->shaders.asteroid, "lightPos");
    glUniformMatrix4fv(loc_model, 1, false, glm::value_ptr(model));
    glUniformMatrix4fv(loc_view, 1, false, glm::value_ptr(view));
    glUniformMatrix4fv(loc_projection, 1, false, glm::value_ptr(projection));
    glUniform3f(loc_lightpos, state->pos.x, state->pos.y, 4.0);
    glUniform1f(loc_health, astr.health / (f32) MAX_ASTEROID_HEALTH);
    glBindTexture(GL_TEXTURE_2D, state->asteroid_texture);
    glDrawElements(GL_TRIANGLES, state->asteroid_indices, GL_UNSIGNED_INT, 0);
  }


  /* Lasers */
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glUseProgram(state->shaders.laser);
  glBindVertexArray(state->vao.laser);
  for (u32 i = 0; i < state->lasers.size(); ++i) {
    struct laser &l = state->lasers[i];
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(l.pos.x, l.pos.y, 0.0f));
    i32 loc_model = glGetUniformLocation(state->shaders.laser, "model");
    i32 loc_view = glGetUniformLocation(state->shaders.laser, "view");
    i32 loc_projection = glGetUniformLocation(state->shaders.laser, "projection");
    glUniformMatrix4fv(loc_model, 1, false, glm::value_ptr(model));
    glUniformMatrix4fv(loc_view, 1, false, glm::value_ptr(view));
    glUniformMatrix4fv(loc_projection, 1, false, glm::value_ptr(projection));
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  /* Ship */
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glUseProgram(state->shaders.ship);
  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(state->pos.x, state->pos.y, 0.0f));
  model = glm::rotate(model, glm::radians(state->dir - 90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
  if (state->movement == MOVEMENT_RIGHT)
    model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  if (state->movement == MOVEMENT_LEFT)
    model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, -1.0f, 0.0f));
  i32 loc_model = glGetUniformLocation(state->shaders.ship, "model");
  i32 loc_view = glGetUniformLocation(state->shaders.ship, "view");
  i32 loc_projection = glGetUniformLocation(state->shaders.ship, "projection");
  glUniformMatrix4fv(loc_model, 1, false, glm::value_ptr(model));
  glUniformMatrix4fv(loc_view, 1, false, glm::value_ptr(view));
  glUniformMatrix4fv(loc_projection, 1, false, glm::value_ptr(projection));
  glBindVertexArray(state->vao.ship);
  glDrawArrays(GL_TRIANGLES, 0, state->boost ? 15 : 9);

}

void
clamp_boundry(struct point *p)
{
  f32 game_boundry = 15.0;
  if (p->x > game_boundry)   p->x = -game_boundry;
  if (p->x < -game_boundry)  p->x = game_boundry;
  if (p->y > game_boundry)   p->y = -game_boundry;
  if (p->y < -game_boundry)  p->y = game_boundry;
}

void
update_scoreboard(void)
{
  printf("health: %d score: %d       \r", state->health, state->score);
  fflush(stdout);
}

char *
read_to_string(const char *path)
{
  FILE *f = fopen(path, "r");
  if (f == NULL)
    err(EXIT_FAILURE, "%s", path);
  fseek(f, 0, SEEK_END);
  size_t flen = ftell(f);
  char *str = new char[flen + 1];
  rewind(f);
  fread(str, sizeof(char), flen, f);
  str[flen] = '\0';
  fclose(f);
  return str;
}

static GLuint
compile_shader(GLenum shader_type, const char *src)
{
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetShaderInfoLog(shader, 512, NULL, info_log);
    errx(EXIT_FAILURE, "Failed to compile shader:\n%s", info_log);
  }

  return shader;
}

static GLuint
link_shaders(GLuint vertex, GLuint fragment)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);
  glDeleteShader(vertex);
  glDeleteShader(fragment);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetProgramInfoLog(program, 512, NULL, info_log);
    errx(EXIT_FAILURE, "Failed to link shader program:\n%s", info_log);
  }

  return program;
}

GLuint
load_shaders(const char *vertex_path, const char *fragment_path)
{
  char *vertex_source = read_to_string(vertex_path);
  char *fragment_source = read_to_string(fragment_path);
  GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_source);
  GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
  delete[] vertex_source;
  delete[] fragment_source;
  return link_shaders(vertex, fragment);
}

void
framebuffer_size_callback(GLFWwindow *, i32 width, i32 height)
{
  state->width = width;
  state->height = height;
  glViewport(0, 0, width, height);
}

f32
to_rads(f32 x)
{
  return x * M_PI / 180.0;
}

f32
aspect_ratio(void)
{
  return state->width / (f32)state->height;
}
