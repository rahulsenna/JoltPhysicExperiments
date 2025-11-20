#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include "graphics_api_gl.h"
#include <stdio.h>
#include <stdlib.h>

struct GLBuffer
{
  GLuint id;
};

struct GLShader
{
  GLuint id;
};

struct GLProgram
{
  GLuint id;
};

struct GLVertexArray
{
  GLuint id;
};

static void gl_set_window_hints()
{
  // OpenGL 4.1 Core Profile (macOS maximum)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS
}

static bool gl_init(GLFWwindow *window)
{
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glEnable(GL_DEPTH_TEST);

  printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
  printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

  return true;
}

static void gl_shutdown()
{
}

static GraphicsBuffer gl_create_buffer(const void *data, size_t size)
{
  GLBuffer *buffer = (GLBuffer *)malloc(sizeof(GLBuffer));
  glGenBuffers(1, &buffer->id);
  glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
  glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  return buffer;
}

static GraphicsShader gl_create_shader(ShaderType type, const char *source)
{
  GLShader *shader = (GLShader *)malloc(sizeof(GLShader));

  GLenum gl_type = (type == SHADER_TYPE_VERTEX) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
  shader->id = glCreateShader(gl_type);
  glShaderSource(shader->id, 1, &source, NULL);
  glCompileShader(shader->id);

  GLint success;
  glGetShaderiv(shader->id, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    char info_log[512];
    glGetShaderInfoLog(shader->id, 512, NULL, info_log);
    fprintf(stderr, "Shader compilation failed: %s\n", info_log);
  }

  return shader;
}

static GraphicsProgram gl_create_program(GraphicsShader vertex, GraphicsShader fragment)
{
  GLProgram *program = (GLProgram *)malloc(sizeof(GLProgram));
  program->id = glCreateProgram();

  GLShader *vs = (GLShader *)vertex;
  GLShader *fs = (GLShader *)fragment;

  glAttachShader(program->id, vs->id);
  glAttachShader(program->id, fs->id);
  glLinkProgram(program->id);

  // Check linking
  GLint success;
  glGetProgramiv(program->id, GL_LINK_STATUS, &success);
  if (!success)
  {
    char info_log[512];
    glGetProgramInfoLog(program->id, 512, NULL, info_log);
    fprintf(stderr, "Program linking failed: %s\n", info_log);
  }

  return program;
}

static GraphicsVertexArray gl_create_vertex_array()
{
  GLVertexArray *vao = (GLVertexArray *)malloc(sizeof(GLVertexArray));
  glGenVertexArrays(1, &vao->id);
  return vao;
}

static void gl_bind_buffer(GraphicsBuffer buffer)
{
  GLBuffer *buf = (GLBuffer *)buffer;
  glBindBuffer(GL_ARRAY_BUFFER, buf->id);
}

static void gl_bind_vertex_array(GraphicsVertexArray vao)
{
  if (vao == nullptr)
  {
    glBindVertexArray(0);
    return;
  }
  GLVertexArray *vertex_array = (GLVertexArray *)vao;
  glBindVertexArray(vertex_array->id);
}

static void gl_use_program(GraphicsProgram program)
{
  GLProgram *prog = (GLProgram *)program;
  glUseProgram(prog->id);
}

static int gl_get_attrib_location(GraphicsProgram program, const char *name)
{
  GLProgram *prog = (GLProgram *)program;
  return glGetAttribLocation(prog->id, name);
}

static int gl_get_uniform_location(GraphicsProgram program, const char *name)
{
  GLProgram *prog = (GLProgram *)program;
  return glGetUniformLocation(prog->id, name);
}

static void gl_enable_vertex_attrib(int location)
{
  glEnableVertexAttribArray(location);
}

static void gl_vertex_attrib_pointer(int location, int size, int stride, size_t offset)
{
  glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, stride, (void *)offset);
}

static void gl_clear(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void gl_viewport(int x, int y, int width, int height)
{
  glViewport(x, y, width, height);
}

static void gl_draw_arrays(int first, int count)
{
  glDrawArrays(GL_TRIANGLES, first, count);
}

static void gl_swap_buffers(GLFWwindow *window)
{
  glfwSwapBuffers(window);
}

static void gl_destroy_buffer(GraphicsBuffer buffer)
{
  GLBuffer *buf = (GLBuffer *)buffer;
  glDeleteBuffers(1, &buf->id);
  free(buf);
}

static void gl_destroy_shader(GraphicsShader shader)
{
  GLShader *sh = (GLShader *)shader;
  glDeleteShader(sh->id);
  free(sh);
}

static void gl_destroy_program(GraphicsProgram program)
{
  GLProgram *prog = (GLProgram *)program;
  glDeleteProgram(prog->id);
  free(prog);
}

static void gl_destroy_vertex_array(GraphicsVertexArray vao)
{
  GLVertexArray *vertex_array = (GLVertexArray *)vao;
  glDeleteVertexArrays(1, &vertex_array->id);
  free(vertex_array);
}

static void gl_set_uniform_mat4(GraphicsProgram program, int location, const float *data)
{
  glUniformMatrix4fv(location, 1, GL_FALSE, data);
}

static void gl_set_uniform_vec3(GraphicsProgram program, int location, const float *data)
{
  if (location == -1)
    return;
  glUniform3fv(location, 1, data);
}

static GraphicsBuffer gl_create_index_buffer(const void *data, size_t size)
{
  GLBuffer *buffer = (GLBuffer *)malloc(sizeof(GLBuffer));
  glGenBuffers(1, &buffer->id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  return buffer;
}

static void gl_bind_index_buffer(GraphicsBuffer buffer)
{
  GLBuffer *buf = (GLBuffer *)buffer;
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->id);
}

static void gl_draw_elements(int count)
{
  glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
}

// Global OpenGL API instance
static GraphicsAPI s_opengl_api = {
    .init = gl_init,
    .shutdown = gl_shutdown,
    .create_buffer = gl_create_buffer,
    .create_shader = gl_create_shader,
    .create_program = gl_create_program,
    .create_vertex_array = gl_create_vertex_array,
    .bind_buffer = gl_bind_buffer,
    .bind_vertex_array = gl_bind_vertex_array,
    .use_program = gl_use_program,
    .get_attrib_location = gl_get_attrib_location,
    .get_uniform_location = gl_get_uniform_location,
    .enable_vertex_attrib = gl_enable_vertex_attrib,
    .vertex_attrib_pointer = gl_vertex_attrib_pointer,
    .clear = gl_clear,
    .viewport = gl_viewport,
    .draw_arrays = gl_draw_arrays,
    .swap_buffers = gl_swap_buffers,
    .destroy_buffer = gl_destroy_buffer,
    .destroy_shader = gl_destroy_shader,
    .destroy_program = gl_destroy_program,
    .destroy_vertex_array = gl_destroy_vertex_array,
    .set_window_hints = gl_set_window_hints,
    .set_uniform_mat4 = gl_set_uniform_mat4,
    .set_uniform_vec3 = gl_set_uniform_vec3,
    .create_index_buffer = gl_create_index_buffer,
    .bind_index_buffer = gl_bind_index_buffer,
    .draw_elements = gl_draw_elements,
};

GraphicsAPI *create_graphics_api_opengl()
{
  return &s_opengl_api;
}
