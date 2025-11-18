#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

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
  api.render = (void (*)(GameMemory *, GLuint, GLint, GLuint))dlsym(api.dll_handle, "game_render");
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

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window = glfwCreateWindow(2880, 1864, "Motorcylce", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);

  const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);

  const GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  const GLint mvp_location = glGetUniformLocation(program, "MVP");
  const GLint vpos_location = glGetAttribLocation(program, "vPos");
  const GLint vcol_location = glGetAttribLocation(program, "vCol");

  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
  glEnableVertexAttribArray(vcol_location);
  glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, col));

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
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    if (game_api.update)
    {
      game_api.update(game_memory, delta_time);
    }

    if (game_api.render)
    {
      game_api.render(game_memory, program, mvp_location, vertex_array);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  if (game_api.shutdown)
  {
    game_api.shutdown(game_memory);
  }
  unload_game_api(&game_api);
  free(game_memory);

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}
