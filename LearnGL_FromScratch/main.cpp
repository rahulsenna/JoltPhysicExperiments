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
    #version 330
    uniform mat4 MVP;
    in vec3 vCol;
    in vec2 vPos;
    out vec3 color;
    void main()
    {
        gl_Position = MVP * vec4(vPos, 0.0, 1.0);
        color = vCol;
    }
)";
const char *fragment_shader_text = R"(
    #version 330
    in vec3 color;
    out vec4 fragment;
    void main()
    {
        fragment = vec4(color, 1.0);
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

  api.dll_handle = dlopen(temp_path, RTLD_NOW | RTLD_LOCAL);
  if (!api.dll_handle)
  {
    fprintf(stderr, "Failed to load %s: %s\n", temp_path, dlerror());
    return api;
  }

  api.init = (void (*)(GameMemory *, arena::MemoryArena *))dlsym(api.dll_handle, "game_init");
  api.update = (void (*)(GameMemory *, float))dlsym(api.dll_handle, "game_update");
  api.render = (void (*)(GameMemory *, RenderContext *))dlsym(api.dll_handle, "game_render");
  api.hot_reloaded = (void (*)(GameMemory *))dlsym(api.dll_handle, "game_hot_reloaded");
  api.shutdown = (void (*)(GameMemory *))dlsym(api.dll_handle, "game_shutdown");

  api.dll_timestamp = get_file_write_time(dll_path);

  printf("Game API loaded from %s\n", temp_path);
  return api;
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

  GraphicsBuffer vertex_buffer = gfx->create_buffer(vertices, sizeof(vertices));

  GraphicsShader vertex_shader = gfx->create_shader(SHADER_TYPE_VERTEX, vertex_shader_text);
  GraphicsShader fragment_shader = gfx->create_shader(SHADER_TYPE_FRAGMENT, fragment_shader_text);

  GraphicsProgram program = gfx->create_program(vertex_shader, fragment_shader);

  int mvp_location = gfx->get_uniform_location(program, "MVP");
  int vpos_location = gfx->get_attrib_location(program, "vPos");
  int vcol_location = gfx->get_attrib_location(program, "vCol");

  GraphicsVertexArray vertex_array = gfx->create_vertex_array();
  gfx->bind_vertex_array(vertex_array);
  gfx->bind_buffer(vertex_buffer);
  gfx->enable_vertex_attrib(vpos_location);
  gfx->vertex_attrib_pointer(vpos_location, 2, sizeof(Vertex), offsetof(Vertex, pos));
  gfx->enable_vertex_attrib(vcol_location);
  gfx->vertex_attrib_pointer(vcol_location, 3, sizeof(Vertex), offsetof(Vertex, col));

  GameMemory *game_memory = (GameMemory *)malloc(sizeof(GameMemory));
  memset(game_memory, 0, sizeof(GameMemory));

  const char *dll_path = "./game.dylib";
  GameAPI game_api = load_game_api(dll_path);

  if (game_api.init)
  {
    game_api.init(game_memory, &arena::GLOBAL_ARENA);
  }

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
          .vertex_array = vertex_array,
          .mvp_location = mvp_location,
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

  gfx->destroy_vertex_array(vertex_array);
  gfx->destroy_program(program);
  gfx->destroy_shader(fragment_shader);
  gfx->destroy_shader(vertex_shader);
  gfx->destroy_buffer(vertex_buffer);

  gfx->shutdown();

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
