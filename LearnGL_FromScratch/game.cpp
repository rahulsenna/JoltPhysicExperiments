#include "game_api.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#define INITIALIZE_MEMORY_ARENA
#include "arena.h"

#include "linmath.h"

#include <stdio.h>

extern "C"
{

  void game_init(GameMemory *memory, arena::MemoryArena *arena)
  {
    printf("Game initialized\n");

    memory->arena = arena;
    memory->rotation_speed = 1.0f;
    memory->last_change_time = 0.0;
    mat4x4_identity(memory->model_matrix);
  }

  void game_update(GameMemory *memory, float delta_time)
  {
    double current_time = glfwGetTime();

    temp_arena temp = begin_temp_memory(1024 * 1024);

    float *temp_calculations = push_temp_array(temp, 100, float);

    memory->rotation_speed = (float)current_time;

    mat4x4_identity(memory->model_matrix);
    mat4x4_rotate_Z(memory->model_matrix, memory->model_matrix, memory->rotation_speed);

    end_temp_memory(temp);
  }

  void game_render(GameMemory *memory, GLuint program, GLint mvp_location, GLuint vertex_array)
  {
    int width, height;
    GLFWwindow *window = glfwGetCurrentContext();
    glfwGetFramebufferSize(window, &width, &height);
    const float ratio = width / (float)height;

    glUseProgram(program);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat *)&memory->model_matrix);
    glBindVertexArray(vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  void game_hot_reloaded(GameMemory *memory)
  {
    printf("===== GAME CODE HOT RELOADED =====\n");
    printf("Arena pointer: %p\n", memory->arena);
    printf("Rotation speed preserved: %f\n", memory->rotation_speed);
  }

  void game_shutdown(GameMemory *memory)
  {
    printf("Game shutdown\n");
    // Don't free the arena - the host owns it
  }

} // extern "C"
