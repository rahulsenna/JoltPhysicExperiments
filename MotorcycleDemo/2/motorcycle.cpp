#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Vehicle/MotorcycleController.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace JPH;
using namespace std;

namespace Layers {
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING = 1;
}

namespace BroadPhaseLayers {
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
}

class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
        switch (inObject1) {
            case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
            case Layers::MOVING: return true;
            default: return false;
        }
    }
};

class BPLayerInterfaceImpl : public BroadPhaseLayerInterface {
public:
    virtual uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
        return inLayer == Layers::NON_MOVING ? BroadPhaseLayers::NON_MOVING : BroadPhaseLayers::MOVING;
    }
};

class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
        return inLayer1 == Layers::NON_MOVING ? inLayer2 == BroadPhaseLayers::MOVING : true;
    }
};

// Vertex shader
const char* vertex_shader_src = R"(
#version 410 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_normal;
out vec3 frag_pos;

void main() {
    frag_pos = vec3(model * vec4(position, 1.0));
    frag_normal = mat3(transpose(inverse(model))) * normal;
    gl_Position = projection * view * vec4(frag_pos, 1.0);
}
)";

// Fragment shader
const char* fragment_shader_src = R"(
#version 410 core
in vec3 frag_normal;
in vec3 frag_pos;

uniform vec3 color;
uniform vec3 light_pos;
uniform vec3 view_pos;

out vec4 frag_color;

void main() {
    vec3 norm = normalize(frag_normal);
    vec3 light_dir = normalize(light_pos - frag_pos);
    
    float ambient = 0.3;
    float diffuse = max(dot(norm, light_dir), 0.0) * 0.7;
    
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0) * 0.5;
    
    vec3 result = (ambient + diffuse + specular) * color;
    frag_color = vec4(result, 1.0);
}
)";

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        cerr << "Shader compilation failed: " << info << endl;
    }
    return shader;
}

GLuint create_program(const char* vs_src, const char* fs_src) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        cerr << "Program linking failed: " << info << endl;
    }
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    return program;
}

struct Mat4 {
    float m[16];
    
    static Mat4 identity() {
        Mat4 result = {};
        result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
        return result;
    }
    
    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 result = {};
        float f = 1.0f / tanf(fov / 2.0f);
        result.m[0] = f / aspect;
        result.m[5] = f;
        result.m[10] = (far + near) / (near - far);
        result.m[11] = -1.0f;
        result.m[14] = (2.0f * far * near) / (near - far);
        return result;
    }
    
    static Mat4 look_at(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz) {
        float fx = cx - ex, fy = cy - ey, fz = cz - ez;
        float len = sqrtf(fx*fx + fy*fy + fz*fz);
        fx /= len; fy /= len; fz /= len;
        
        float sx = fy*uz - fz*uy;
        float sy = fz*ux - fx*uz;
        float sz = fx*uy - fy*ux;
        len = sqrtf(sx*sx + sy*sy + sz*sz);
        sx /= len; sy /= len; sz /= len;
        
        float upx = sy*fz - sz*fy;
        float upy = sz*fx - sx*fz;
        float upz = sx*fy - sy*fx;
        
        Mat4 result = {};
        result.m[0] = sx; result.m[4] = sy; result.m[8] = sz;
        result.m[1] = upx; result.m[5] = upy; result.m[9] = upz;
        result.m[2] = -fx; result.m[6] = -fy; result.m[10] = -fz;
        result.m[12] = -(sx*ex + sy*ey + sz*ez);
        result.m[13] = -(upx*ex + upy*ey + upz*ez);
        result.m[14] = fx*ex + fy*ey + fz*ez;
        result.m[15] = 1.0f;
        return result;
    }
    
    static Mat4 translate(float x, float y, float z) {
        Mat4 result = identity();
        result.m[12] = x;
        result.m[13] = y;
        result.m[14] = z;
        return result;
    }
    
    static Mat4 rotate(float angle, float x, float y, float z) {
        float c = cosf(angle);
        float s = sinf(angle);
        float len = sqrtf(x*x + y*y + z*z);
        x /= len; y /= len; z /= len;
        
        Mat4 result = {};
        result.m[0] = x*x*(1-c) + c;
        result.m[1] = y*x*(1-c) + z*s;
        result.m[2] = x*z*(1-c) - y*s;
        result.m[4] = x*y*(1-c) - z*s;
        result.m[5] = y*y*(1-c) + c;
        result.m[6] = y*z*(1-c) + x*s;
        result.m[8] = x*z*(1-c) + y*s;
        result.m[9] = y*z*(1-c) - x*s;
        result.m[10] = z*z*(1-c) + c;
        result.m[15] = 1.0f;
        return result;
    }
    
    Mat4 operator*(const Mat4& other) const {
        Mat4 result = {};
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    result.m[i*4 + j] += m[i*4 + k] * other.m[k*4 + j];
        return result;
    }
};

struct Mesh {
    GLuint vao, vbo, ebo;
    int index_count;
    
    static Mesh create_box(float w, float h, float d) {
        float vertices[] = {
            // positions          // normals
            -w,-h, d,  0, 0, 1,   w,-h, d,  0, 0, 1,   w, h, d,  0, 0, 1,  -w, h, d,  0, 0, 1, // front
            -w,-h,-d,  0, 0,-1,  -w, h,-d,  0, 0,-1,   w, h,-d,  0, 0,-1,   w,-h,-d,  0, 0,-1, // back
            -w, h,-d,  0, 1, 0,  -w, h, d,  0, 1, 0,   w, h, d,  0, 1, 0,   w, h,-d,  0, 1, 0, // top
            -w,-h,-d,  0,-1, 0,   w,-h,-d,  0,-1, 0,   w,-h, d,  0,-1, 0,  -w,-h, d,  0,-1, 0, // bottom
             w,-h,-d,  1, 0, 0,   w, h,-d,  1, 0, 0,   w, h, d,  1, 0, 0,   w,-h, d,  1, 0, 0, // right
            -w,-h,-d, -1, 0, 0,  -w,-h, d, -1, 0, 0,  -w, h, d, -1, 0, 0,  -w, h,-d, -1, 0, 0, // left
        };
        
        unsigned int indices[] = {
            0,1,2, 0,2,3,   4,5,6, 4,6,7,   8,9,10, 8,10,11,
            12,13,14, 12,14,15,   16,17,18, 16,18,19,   20,21,22, 20,22,23
        };
        
        Mesh mesh;
        glGenVertexArrays(1, &mesh.vao);
        glGenBuffers(1, &mesh.vbo);
        glGenBuffers(1, &mesh.ebo);
        
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        
        mesh.index_count = 36;
        return mesh;
    }
    
    static Mesh create_ground(float size) {
        float vertices[] = {
            -size, 0, -size,  0, 1, 0,
             size, 0, -size,  0, 1, 0,
             size, 0,  size,  0, 1, 0,
            -size, 0,  size,  0, 1, 0,
        };
        
        unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
        
        Mesh mesh;
        glGenVertexArrays(1, &mesh.vao);
        glGenBuffers(1, &mesh.vbo);
        glGenBuffers(1, &mesh.ebo);
        
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        
        mesh.index_count = 6;
        return mesh;
    }
    
    void draw() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

int main() {
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    
    // Request OpenGL 4.1 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Motorcycle Simulator - OpenGL 4.1", NULL, NULL);
    if (!window) {
        cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    
    // Create shader program
    GLuint shader = create_program(vertex_shader_src, fragment_shader_src);
    
    // Create meshes
    Mesh box_mesh = Mesh::create_box(0.2f, 0.3f, 0.4f);
    Mesh ground_mesh = Mesh::create_ground(50.0f);
    
    // Initialize Jolt (same as before)
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();
    
    TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
    JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    
    ObjectLayerPairFilterImpl object_filter;
    BPLayerInterfaceImpl bp_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_bp_filter;
    
    PhysicsSystem physics_system;
    physics_system.Init(1024, 0, 1024, 1024, bp_interface, object_vs_bp_filter, object_filter);
    
    BodyInterface& body_interface = physics_system.GetBodyInterface();
    
    // Create ground
    BoxShapeSettings ground_shape_settings(Vec3(50.0f, 1.0f, 50.0f));
    ShapeRefC ground_shape = ground_shape_settings.Create().Get();
    BodyCreationSettings ground_settings(ground_shape, RVec3(0.0, -1.0, 0.0), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
    Body* ground = body_interface.CreateBody(ground_settings);
    body_interface.AddBody(ground->GetID(), EActivation::DontActivate);
    
    // Create motorcycle
    const float hw = 0.2f, hh = 0.3f, hl = 0.4f;
    RefConst<Shape> motorcycle_shape = OffsetCenterOfMassShapeSettings(
        Vec3(0, -hh, 0), new BoxShape(Vec3(hw, hh, hl))
    ).Create().Get();
    
    BodyCreationSettings motorcycle_body_settings(motorcycle_shape, RVec3(0, 2, 0), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    motorcycle_body_settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
    motorcycle_body_settings.mMassPropertiesOverride.mMass = 240.0f;
    
    Body* motorcycle_body = body_interface.CreateBody(motorcycle_body_settings);
    body_interface.AddBody(motorcycle_body->GetID(), EActivation::Activate);
    
    // Create vehicle constraint (same setup as before)
    VehicleConstraintSettings vehicle;
    
    WheelSettingsWV* front = new WheelSettingsWV;
    front->mPosition = Vec3(0.0f, -0.9f * hh, 0.75f);
    front->mMaxSteerAngle = DegreesToRadians(30);
    front->mRadius = 0.31f;
    front->mWidth = 0.05f;
    front->mSuspensionMinLength = 0.3f;
    front->mSuspensionMaxLength = 0.5f;
    front->mSuspensionSpring.mFrequency = 1.5f;
    front->mMaxBrakeTorque = 500.0f;
    const float caster_angle = DegreesToRadians(30);
    front->mSuspensionDirection = Vec3(0, -1, Tan(caster_angle)).Normalized();
    front->mSteeringAxis = -front->mSuspensionDirection;
    
    WheelSettingsWV* back = new WheelSettingsWV;
    back->mPosition = Vec3(0.0f, -0.9f * hh, -0.75f);
    back->mMaxSteerAngle = 0.0f;
    back->mRadius = 0.31f;
    back->mWidth = 0.05f;
    back->mSuspensionMinLength = 0.3f;
    back->mSuspensionMaxLength = 0.5f;
    back->mSuspensionSpring.mFrequency = 2.0f;
    back->mMaxBrakeTorque = 250.0f;
    
    vehicle.mWheels = { front, back };
    
    MotorcycleControllerSettings* controller = new MotorcycleControllerSettings;
    controller->mEngine.mMaxTorque = 150.0f;
    controller->mEngine.mMinRPM = 1000.0f;
    controller->mEngine.mMaxRPM = 10000.0f;
    controller->mTransmission.mShiftDownRPM = 2000.0f;
    controller->mTransmission.mShiftUpRPM = 8000.0f;
    controller->mTransmission.mGearRatios = { 2.27f, 1.63f, 1.3f, 1.09f, 0.96f, 0.88f };
    controller->mTransmission.mClutchStrength = 2.0f;
    controller->mDifferentials.resize(1);
    controller->mDifferentials[0].mLeftWheel = -1;
    controller->mDifferentials[0].mRightWheel = 1;
    controller->mDifferentials[0].mDifferentialRatio = 1.93f * 40.0f / 16.0f;
    vehicle.mController = controller;
    
    VehicleConstraint* vehicle_constraint = new VehicleConstraint(*motorcycle_body, vehicle);
    vehicle_constraint->SetVehicleCollisionTester(new VehicleCollisionTesterRay(Layers::MOVING));
    physics_system.AddConstraint(vehicle_constraint);
    physics_system.AddStepListener(vehicle_constraint);
   
    // Main loop
    const float dt = 1.0f / 60.0f;
    float camera_distance = 10.0f;
    float camera_height = 5.0f;

    // Create cylinder mesh for wheels
    Mesh wheel_mesh = Mesh::create_box(0.05f, 0.31f, 0.31f); // width, radius, radius (we'll rotate it)

    cout << "Controls: Arrow Up = Forward, Down = Reverse, Left/Right = Steer, Space = Brake, ESC = Exit\n";

    glEnable(GL_DEPTH_TEST);

    float previous_forward = 0.0f;
    float steer_input = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        // Input
        float forward = 0.0f, brake = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) forward = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) forward = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) brake = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        // Smooth steering
        float steer_target = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) steer_target = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) steer_target = 1.0f;

        const float steer_speed = 4.0f;
        if (steer_target > steer_input)
            steer_input = min(steer_input + steer_speed * dt, steer_target);
        else if (steer_target < steer_input)
            steer_input = max(steer_input - steer_speed * dt, steer_target);

        // Handle reverse direction (need to brake first)
        if (previous_forward * forward < 0.0f) {
            Vec3 local_vel = motorcycle_body->GetRotation().Conjugated() * motorcycle_body->GetLinearVelocity();
            float vel_z = local_vel.GetZ();
            if ((forward > 0.0f && vel_z < -0.1f) || (forward < 0.0f && vel_z > 0.1f)) {
                forward = 0.0f;
                brake = 1.0f;
            } else {
                previous_forward = forward;
            }
        }

        // Update physics
        MotorcycleController* mc = static_cast<MotorcycleController*>(vehicle_constraint->GetController());
        mc->SetDriverInput(forward, steer_input, brake, false);
        mc->EnableLeanController(true); // ENABLE LEAN CONTROLLER

        if (forward != 0.0f || steer_input != 0.0f || brake != 0.0f)
            body_interface.ActivateBody(motorcycle_body->GetID());

        physics_system.Update(dt, 1, &temp_allocator, &job_system);

        // Render
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Setup matrices
        Mat4 projection = Mat4::perspective(1.047f, (float)width / height, 0.1f, 1000.0f);

        RVec3 bike_pos = motorcycle_body->GetPosition();
        Quat bike_rot = motorcycle_body->GetRotation();
        Vec3 bike_forward = bike_rot.RotateAxisZ();

        float cam_x = bike_pos.GetX() - bike_forward.GetX() * camera_distance;
        float cam_y = bike_pos.GetY() + camera_height;
        float cam_z = bike_pos.GetZ() - bike_forward.GetZ() * camera_distance;

        Mat4 view = Mat4::look_at(cam_x, cam_y, cam_z, 
                bike_pos.GetX(), bike_pos.GetY(), bike_pos.GetZ(),
                0, 1, 0);

        glUseProgram(shader);

        GLint model_loc = glGetUniformLocation(shader, "model");
        GLint view_loc = glGetUniformLocation(shader, "view");
        GLint proj_loc = glGetUniformLocation(shader, "projection");
        GLint color_loc = glGetUniformLocation(shader, "color");
        GLint light_loc = glGetUniformLocation(shader, "light_pos");
        GLint view_pos_loc = glGetUniformLocation(shader, "view_pos");

        glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.m);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection.m);
        glUniform3f(light_loc, 10.0f, 20.0f, 10.0f);
        glUniform3f(view_pos_loc, cam_x, cam_y, cam_z);

        // Draw ground
        Mat4 ground_model = Mat4::translate(0, -1, 0);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, ground_model.m);
        glUniform3f(color_loc, 0.2f, 0.3f, 0.2f);
        ground_mesh.draw();

        // Draw motorcycle body
        Mat4 bike_model = Mat4::identity();
        float qw = bike_rot.GetW();
        float qx = bike_rot.GetX();
        float qy = bike_rot.GetY();
        float qz = bike_rot.GetZ();

        bike_model.m[0] = 1.0f - 2.0f * (qy*qy + qz*qz);
        bike_model.m[1] = 2.0f * (qx*qy + qw*qz);
        bike_model.m[2] = 2.0f * (qx*qz - qw*qy);
        bike_model.m[4] = 2.0f * (qx*qy - qw*qz);
        bike_model.m[5] = 1.0f - 2.0f * (qx*qx + qz*qz);
        bike_model.m[6] = 2.0f * (qy*qz + qw*qx);
        bike_model.m[8] = 2.0f * (qx*qz + qw*qy);
        bike_model.m[9] = 2.0f * (qy*qz - qw*qx);
        bike_model.m[10] = 1.0f - 2.0f * (qx*qx + qy*qy);
        bike_model.m[12] = bike_pos.GetX();
        bike_model.m[13] = bike_pos.GetY();
        bike_model.m[14] = bike_pos.GetZ();
        bike_model.m[15] = 1.0f;

        glUniformMatrix4fv(model_loc, 1, GL_FALSE, bike_model.m);
        glUniform3f(color_loc, 0.8f, 0.2f, 0.2f);
        box_mesh.draw();

        // Draw wheels
        glUniform3f(color_loc, 0.1f, 0.1f, 0.1f);
        for (uint w = 0; w < 2; w++) {
            RMat44 wheel_transform = vehicle_constraint->GetWheelWorldTransform(w, Vec3::sAxisY(), Vec3::sAxisX());

            Mat4 wheel_model = Mat4::identity();
            // for (int i = 0; i < 12; i++) {
                // wheel_model.m[i] = wheel_transform.GetColumn3(i / 4)[i % 4];
            // }
            //
            // wheel_model.m[12] = wheel_transform.GetTranslation().GetX();
            // wheel_model.m[13] = wheel_transform.GetTranslation().GetY();
            // wheel_model.m[14] = wheel_transform.GetTranslation().GetZ();
            // wheel_model.m[15] = 1.0f;
            
            // Draw wheels
            glUniform3f(color_loc, 0.1f, 0.1f, 0.1f);
            for (uint w = 0; w < 2; w++) {
                RMat44 wheel_transform = vehicle_constraint->GetWheelWorldTransform(w, Vec3::sAxisY(), Vec3::sAxisX());

                Mat4 wheel_model = Mat4::identity();

                // Extract 3x3 rotation matrix
                Vec3 col0 = wheel_transform.GetAxisX();
                Vec3 col1 = wheel_transform.GetAxisY();
                Vec3 col2 = wheel_transform.GetAxisZ();

                wheel_model.m[0] = col0.GetX();
                wheel_model.m[1] = col0.GetY();
                wheel_model.m[2] = col0.GetZ();
                wheel_model.m[3] = 0.0f;

                wheel_model.m[4] = col1.GetX();
                wheel_model.m[5] = col1.GetY();
                wheel_model.m[6] = col1.GetZ();
                wheel_model.m[7] = 0.0f;

                wheel_model.m[8] = col2.GetX();
                wheel_model.m[9] = col2.GetY();
                wheel_model.m[10] = col2.GetZ();
                wheel_model.m[11] = 0.0f;

                // Extract translation
                RVec3 pos = wheel_transform.GetTranslation();
                wheel_model.m[12] = pos.GetX();
                wheel_model.m[13] = pos.GetY();
                wheel_model.m[14] = pos.GetZ();
                wheel_model.m[15] = 1.0f;

                glUniformMatrix4fv(model_loc, 1, GL_FALSE, wheel_model.m);
                wheel_mesh.draw();
            }


            glUniformMatrix4fv(model_loc, 1, GL_FALSE, wheel_model.m);
            wheel_mesh.draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    
    // Cleanup
    physics_system.RemoveStepListener(vehicle_constraint);
    physics_system.RemoveConstraint(vehicle_constraint);
    body_interface.RemoveBody(motorcycle_body->GetID());
    body_interface.DestroyBody(motorcycle_body->GetID());
    body_interface.RemoveBody(ground->GetID());
    body_interface.DestroyBody(ground->GetID());
    
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
