#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#define INITIALIZE_MEMORY_ARENA
#include "arena.h"
#include "linmath.h"
#include "game_api.h"
#include "graphics_api.h"
#include "graphics_api_gl.h"

typedef struct Vertex
{
  vec2 pos;
  vec3 col;
} Vertex;

static const Vertex vertices[3] =
    {{{-0.6f, -0.4f}, {1.f, 0.f, 0.f}},
     {{0.6f, -0.4f}, {0.f, 1.f, 0.f}},
     {{0.f, 0.6f}, {0.f, 0.f, 1.f}}};

const char *vertex_shader_text = R"(
#version 410 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_normal;
out vec3 frag_color;
out vec3 frag_pos;

void main() {
    frag_pos = vec3(model * vec4(position, 1.0));
    frag_normal = mat3(transpose(inverse(model))) * normal;
    frag_color = color;
    gl_Position = projection * view * vec4(frag_pos, 1.0);
}
)";
const char *fragment_shader_text = R"(
#version 410 core
in vec3 frag_normal;
in vec3 frag_color;
in vec3 frag_pos;

uniform vec3 light_pos;
uniform vec3 view_pos;

out vec4 out_color;

void main() {
    vec3 norm = normalize(frag_normal);
    vec3 light_dir = normalize(light_pos - frag_pos);
    
    float ambient = 0.3;
    float diffuse = max(dot(norm, light_dir), 0.0) * 0.7;
    
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0) * 0.5;
    
    vec3 result = (ambient + diffuse + specular) * frag_color;
    out_color = vec4(result, 1.0);
}
)";

static void error_callback(int error, const char *description)
{
  fprintf(stderr, "Error: %s\n", description);
}
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

time_t get_file_write_time(const char *path)
{
  struct stat file_stat;
  if (stat(path, &file_stat) == 0)
  {
    return file_stat.st_mtime;
  }
  return 0;
}

GameAPI load_game_api(const char *dll_path)
{
  GameAPI api = {};

  char temp_path[256];
  snprintf(temp_path, sizeof(temp_path), "%s.temp%ld", dll_path, (long)time(NULL));

  char cmd[512];
  snprintf(cmd, sizeof(cmd), "cp %s %s 2>/dev/null", dll_path, temp_path);
  system(cmd);

   // Copy the dSYM bundle for debugging support
  char dsym_src[256];
  char dsym_dst[256];
  snprintf(dsym_src, sizeof(dsym_src), "%s.dSYM", dll_path);
  snprintf(dsym_dst, sizeof(dsym_dst), "%s.dSYM", temp_path);
  
  char dsym_cmd[512];
  snprintf(dsym_cmd, sizeof(dsym_cmd), "cp -r %s %s 2>/dev/null", dsym_src, dsym_dst);
  system(dsym_cmd);

  api.dll_handle = dlopen(temp_path, RTLD_NOW | RTLD_LOCAL);
  if (!api.dll_handle)
  {
    fprintf(stderr, "Failed to load %s: %s\n", temp_path, dlerror());
    return api;
  }

  api.init = (void (*)(GameMemory *, arena::MemoryArena *, GraphicsAPI *))dlsym(api.dll_handle, "game_init");
  api.update = (void (*)(GameMemory *, float))dlsym(api.dll_handle, "game_update");
  api.render = (void (*)(GameMemory *, RenderContext *))dlsym(api.dll_handle, "game_render");
  api.hot_reloaded = (void (*)(GameMemory *))dlsym(api.dll_handle, "game_hot_reloaded");
  api.shutdown = (void (*)(GameMemory *))dlsym(api.dll_handle, "game_shutdown");

  api.dll_timestamp = get_file_write_time(dll_path);

  printf("Game API loaded from %s\n", temp_path);
  return api;
}

void cleanup_old_temp_files(const char *dll_path)
{
  char pattern[256];
  snprintf(pattern, sizeof(pattern), "%s.temp*", dll_path);

  char cleanup_cmd[512];
  snprintf(cleanup_cmd, sizeof(cleanup_cmd),
           "ls -t %s.temp* 2>/dev/null | tail -n +6 | xargs rm -rf 2>/dev/null",
           dll_path);
  system(cleanup_cmd);
}

void unload_game_api(GameAPI *api)
{
  if (api->dll_handle)
  {
    dlclose(api->dll_handle);
    api->dll_handle = nullptr;
  }
}

int main()
{
  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  GraphicsAPI *gfx = create_graphics_api_opengl();

  gfx->set_window_hints();

  GLFWwindow *window = glfwCreateWindow(2880, 1864, "Motorcycle Adventure", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);

  if (!gfx->init(window))
  {
    fprintf(stderr, "Failed to initialize graphics API\n");
    exit(EXIT_FAILURE);
  }

  GraphicsShader vertex_shader = gfx->create_shader(SHADER_TYPE_VERTEX, vertex_shader_text);
  GraphicsShader fragment_shader = gfx->create_shader(SHADER_TYPE_FRAGMENT, fragment_shader_text);
  GraphicsProgram program = gfx->create_program(vertex_shader, fragment_shader);

  GameMemory *game_memory = (GameMemory *)malloc(sizeof(GameMemory));
  memset(game_memory, 0, sizeof(GameMemory));

  const char *dll_path = "./game.dylib";
  GameAPI game_api = load_game_api(dll_path);

  game_api.init(game_memory, &arena::GLOBAL_ARENA, gfx);

  double last_time = glfwGetTime();
  double last_check_time = last_time;

  while (!glfwWindowShouldClose(window))
  {
    double current_time = glfwGetTime();
    float delta_time = (float)(current_time - last_time);
    last_time = current_time;

    if (current_time - last_check_time > 0.5)
    {
      last_check_time = current_time;

      time_t new_timestamp = get_file_write_time(dll_path);
      if (new_timestamp > game_api.dll_timestamp)
      {
        printf("\n>>> Detected game.dylib change, reloading...\n");

        unload_game_api(&game_api);
        usleep(100000); // 100ms delay

        game_api = load_game_api(dll_path);

        if (game_api.hot_reloaded)
        {
          game_api.hot_reloaded(game_memory);
        }

        printf(">>> Hot reload complete!\n\n");
      }
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    gfx->viewport(0, 0, width, height);
    gfx->clear(0.0f, 0.0f, 0.0f, 1.0f);    

    if (game_api.update)
    {
      game_api.update(game_memory, delta_time);
    }

    if (game_api.render)
    {
      RenderContext ctx = {
          .program = program,
          .width = width,
          .height = height,
          .gfx = gfx};
      game_api.render(game_memory, &ctx);
    }

    gfx->swap_buffers(window);
    glfwPollEvents();
  }

  if (game_api.shutdown)
  {
    game_api.shutdown(game_memory);
  }
  unload_game_api(&game_api);
  free(game_memory);
  cleanup_old_temp_files(dll_path);

  gfx->shutdown();

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
