#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#include <vector>
#include <cmath>
#include <iostream>
#include "mesh.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"



const char *vertex_shader = R"(
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

const char *fragment_shader = R"(
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

GLuint compile_shader(GLenum type, const char *source)
{
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    char info[512];
    glGetShaderInfoLog(shader, 512, nullptr, info);
    std::cerr << "Shader error: " << info << std::endl;
  }
  return shader;
}

GLuint create_program()
{
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  return program;
}

// Simple 4x4 matrix helpers
struct Mat4
{
  float m[16] = {0};

  static Mat4 identity()
  {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
  }

  static Mat4 perspective(float fov, float aspect, float near, float far)
  {
    Mat4 r;
    float f = 1.0f / tanf(fov / 2.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
  }

  static Mat4 look_at(float ex, float ey, float ez, float cx, float cy, float cz)
  {
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    float len = sqrtf(fx * fx + fy * fy + fz * fz);
    fx /= len;
    fy /= len;
    fz /= len;

    float sx = fy, sy = -fx, sz = 0;
    len = sqrtf(sx * sx + sy * sy + sz * sz);
    if (len > 0.001f)
    {
      sx /= len;
      sy /= len;
      sz /= len;
    }

    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    Mat4 r;
    r.m[0] = sx;
    r.m[4] = sy;
    r.m[8] = sz;
    r.m[1] = ux;
    r.m[5] = uy;
    r.m[9] = uz;
    r.m[2] = -fx;
    r.m[6] = -fy;
    r.m[10] = -fz;
    r.m[12] = -(sx * ex + sy * ey + sz * ez);
    r.m[13] = -(ux * ex + uy * ey + uz * ez);
    r.m[14] = fx * ex + fy * ey + fz * ez;
    r.m[15] = 1.0f;
    return r;
  }

  static Mat4 translate(float x, float y, float z)
  {
    Mat4 r = identity();
    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;
    return r;
  }
};

int main()
{
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "3D Shapes Demo", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Initialize ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 410");

  std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

  GLuint program = create_program();

  // Create different shapes
  Mesh ground = Mesh::create_ground(20.0f);
  Mesh box = Mesh::create_box(0.5f, 0.5f, 0.5f, 0.8f, 0.2f, 0.2f);
  Mesh sphere = Mesh::create_sphere(0.5f, 36, 18, 0.2f, 0.2f, 0.8f);
  Mesh cylinder = Mesh::create_cylinder(0.3f, 1.0f, 36, 0.8f, 0.8f, 0.2f);
  Mesh cone = Mesh::create_cone(0.5f, 1.0f, 36, 0.8f, 0.4f, 0.2f);

  Mesh cylinder2 = Mesh::create_cylinder(0.3f, 1.0f, 36, 0.8f, 0.8f, 0.2f);
  float c_t_2x = 1.0f, c_t_2y = 0.5f, c_t_2z = 0.0f;

  glEnable(GL_DEPTH_TEST);

  float time = 0.0f;

  // Before your main loop, add camera variables
  float cam_x = 0.040f, cam_y = 2.5f, cam_z = -10.0f;
  float target_x = 0.0f, target_y = 0.0f, target_z = 0.0f;

  while (!glfwWindowShouldClose(window))
  {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GLFW_TRUE);

    time += 0.016f;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Mat4 projection = Mat4::perspective(1.047f, (float)width / height, 0.1f, 100.0f);
    // Mat4 view = Mat4::look_at(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

    // In your main loop, replace the static Mat4::look_at with:

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Camera Controls");

    ImGui::Text("Camera Position");
    ImGui::DragFloat("Cam X", &cam_x, 0.1f, -20.0f, 20.0f);
    ImGui::DragFloat("Cam Y", &cam_y, 0.1f, 0.1f, 20.0f);
    ImGui::DragFloat("Cam Z", &cam_z, 0.1f, -20.0f, 20.0f);

    ImGui::Separator();


    ImGui::Text("Cylinder 2");
    ImGui::DragFloat("c_t_2x", &c_t_2x, 0.1f, -20.0f, 20.0f);
    ImGui::DragFloat("c_t_2y", &c_t_2y, 0.1f, 0.1f, 20.0f);
    ImGui::DragFloat("c_t_2z", &c_t_2z, 0.1f, -20.0f, 20.0f);

    ImGui::Separator();

    ImGui::Text("Camera Target");
    ImGui::DragFloat("Target X", &target_x, 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat("Target Y", &target_y, 0.1f, -5.0f, 5.0f);
    ImGui::DragFloat("Target Z", &target_z, 0.1f, -10.0f, 10.0f);

    ImGui::Separator();

    // Camera presets
    ImGui::Text("Presets");
    if (ImGui::Button("Top View"))
    {
      cam_x = 0.0f;
      cam_y = 10.0f;
      cam_z = 0.0f;
      target_x = 0.0f;
      target_y = 0.0f;
      target_z = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Side View"))
    {
      cam_x = 10.0f;
      cam_y = 2.0f;
      cam_z = 0.0f;
      target_x = 0.0f;
      target_y = 0.0f;
      target_z = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Front View"))
    {
      cam_x = 0.0f;
      cam_y = 2.0f;
      cam_z = 10.0f;
      target_x = 0.0f;
      target_y = 0.0f;
      target_z = 0.0f;
    }

    if (ImGui::Button("Isometric"))
    {
      cam_x = 8.0f;
      cam_y = 5.0f;
      cam_z = 8.0f;
      target_x = 0.0f;
      target_y = 0.0f;
      target_z = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
      cam_x = cam_y = cam_z = 1.0f;
      target_x = target_y = target_z = 0.0f;
    }

    ImGui::Separator();

    float distance = sqrtf(
        (cam_x - target_x) * (cam_x - target_x) +
        (cam_y - target_y) * (cam_y - target_y) +
        (cam_z - target_z) * (cam_z - target_z));
    ImGui::Text("Distance: %.2f", distance);

    // FOV control
    static float fov_degrees = 60.0f;
    if (ImGui::SliderFloat("FOV", &fov_degrees, 30.0f, 120.0f))
    {
      // Update projection matrix with new FOV
    }

    ImGui::End();

    // Use the slider values for view matrix
    Mat4 projection = Mat4::perspective(1.047f, (float)width / height, 0.1f, 100.0f);
    Mat4 view = Mat4::look_at(cam_x, cam_y, cam_z, target_x, target_y, target_z);


    glUseProgram(program);

    GLint model_loc = glGetUniformLocation(program, "model");
    GLint view_loc = glGetUniformLocation(program, "view");
    GLint proj_loc = glGetUniformLocation(program, "projection");
    GLint light_loc = glGetUniformLocation(program, "light_pos");
    GLint view_pos_loc = glGetUniformLocation(program, "view_pos");

    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection.m);
    glUniform3f(light_loc, 5.0f, 10.0f, 5.0f);
    glUniform3f(view_pos_loc, 8.0f, 5.0f, 8.0f);

    // Draw ground
    Mat4 ground_model = Mat4::identity();
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, ground_model.m);
    ground.draw();

    // Draw box
    Mat4 box_model = Mat4::translate(-3.0f, 0.5f, 0.0f);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, box_model.m);
    box.draw();

    // Draw sphere
    Mat4 sphere_model = Mat4::translate(-1.0f, 0.5f, 0.0f);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, sphere_model.m);
    sphere.draw();

    // Draw cylinder
    Mat4 cylinder_model = Mat4::translate(1.0f, 0.5f, 0.0f);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, cylinder_model.m);
    cylinder.draw();

    
    Mat4 cylinder_model2 = Mat4::translate(c_t_2x, c_t_2y, c_t_2z);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, cylinder_model2.m);
    cylinder2.draw();

    // Draw cone
    Mat4 cone_model = Mat4::translate(3.0f, 0.5f, 0.0f);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, cone_model.m);
    cone.draw();

    // Render ImGui on top
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
