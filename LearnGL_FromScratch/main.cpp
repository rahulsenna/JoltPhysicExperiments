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
  api.update = (void (*)(GameMemory *, GameInput*))dlsym(api.dll_handle, "game_update");
  api.render = (void (*)(GameMemory *))dlsym(api.dll_handle, "game_render");
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
void get_inputs(GameInput *input, GLFWwindow *window);
void mouse_callback(GLFWwindow *window, r64 xpos, r64 ypos);

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

  GameMemory *game_memory = push_struct(arena, GameMemory);
  game_memory->arena = arena;
  game_memory->gfx = gfx;

  GameInput *input = push_struct(arena, GameInput);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // lock cursor & enable relative mouse mode
  glfwSetWindowUserPointer(window, input); // tell GLFW where GameInput struct lives
  glfwSetCursorPosCallback(window, mouse_callback);

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

    glfwGetFramebufferSize(window, &game_memory->width, &game_memory->height);
    gfx->viewport(0, 0, game_memory->width, game_memory->height);
    gfx->clear(0.0f, 0.0f, 0.0f, 1.0f);
    
    glfwPollEvents();
    if (game_api.update)
    {
      input->deltat_for_frame = delta_time;
      get_inputs(input, window);
      game_api.update(game_memory, input);
      
      // reset mouse pointer deltas after update
      input->mouse_x = 0.0;
      input->mouse_y = 0.0;
    }

    if (game_api.render)
    {
      game_api.render(game_memory);
    }

    gfx->swap_buffers(window);
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

void get_inputs(GameInput *input, GLFWwindow *window)
{
  input->controllers.move_up.down = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
  input->controllers.move_down.down = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
  input->controllers.move_left.down = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
  input->controllers.move_right.down = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

  input->controllers.action_up.down = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
  input->controllers.action_down.down = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
  input->controllers.action_left.down = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
  input->controllers.action_right.down = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;

  input->controllers.left_shoulder.down = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
  input->controllers.right_shoulder.down = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

  input->controllers.start.down = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
  input->controllers.back.down = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

  // Mouse buttons
  input->mouse_buttons[0].down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  input->mouse_buttons[1].down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  input->mouse_buttons[2].down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
}

r64 g_last_mouse_x = 0, g_last_mouse_y = 0;
bool g_first_mouse = true;

void mouse_callback(GLFWwindow *window, r64 xpos, r64 ypos)
{
  if (g_first_mouse)
  {
    g_last_mouse_x = xpos;
    g_last_mouse_y = ypos;
    g_first_mouse = false;
  }

  r64 xoffset = xpos - g_last_mouse_x;
  r64 yoffset = ypos - g_last_mouse_y;

  g_last_mouse_x = xpos;
  g_last_mouse_y = ypos;

  GameInput *input = (GameInput *)glfwGetWindowUserPointer(window);
  input->mouse_x = xoffset;
  input->mouse_y = yoffset;
}