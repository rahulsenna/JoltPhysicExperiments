#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#define ARENA_IMPLEMENTATION
#include "arena2.h"

#include "linmath.h"
#include "game_api.h"
#include "graphics_api.h"
#include "graphics_api_gl.h"
#include "shader.h"

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

  api.init = (void (*)(GameMemory *))dlsym(api.dll_handle, "game_init");
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
  system("rm -rf game.dylib.te*");
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
  Arena *arena = arena_alloc(TB(64), KB(64), 0);

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

  Shader shader = Shader::create_basic(arena, gfx);

  GameMemory *game_memory = push_struct(arena, GameMemory);
  game_memory->arena = arena;
  game_memory->gfx = gfx;

  const char *dll_path = "./game.dylib";
  GameAPI game_api = load_game_api(dll_path);

  game_api.init(game_memory);

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
          .shader = &shader,
          .width = width,
          .height = height,
      };
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
  cleanup_old_temp_files(dll_path);

  gfx->shutdown();

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
