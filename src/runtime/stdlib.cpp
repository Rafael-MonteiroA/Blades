#include "runtime/stdlib.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <raylib.h>
namespace blades
{

static Value stdlib_print(const std::vector<Value>& args)
{
    for (size_t i = 0; i < args.size(); ++i)
    {
        std::cout << to_string(args[i]);
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return Value(Nil{});
}

static Value stdlib_array_push(const std::vector<Value>& args)
{
    if (args.size() == 2 && args[0].is_array())
    {
        auto arr = args[0].as_array();
        arr->elements.push_back(args[1]);
        return Value(Nil{});
    }
    return Value(Nil{});
}

static Value stdlib_array_pop(const std::vector<Value>& args)
{
    if (args.size() == 1 && args[0].is_array())
    {
        auto arr = args[0].as_array();
        if (!arr->elements.empty())
        {
            Value val = arr->elements.back();
            arr->elements.pop_back();
            return val;
        }
    }
    return Value(Nil{});
}

static Value stdlib_clock(const std::vector<Value>& /*args*/)
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return Value(std::chrono::duration_cast<std::chrono::duration<double>>(now).count());
}

static Value stdlib_typeof(const std::vector<Value>& args)
{
    if (args.empty()) return Value("nil");
    
    const Value& val = args[0];
    if (val.is_nil()) return Value("nil");
    if (val.is_int()) return Value("int");
    if (val.is_double()) return Value("double");
    if (val.is_bool()) return Value("bool");
    if (val.is_string()) return Value("string");
    if (val.is_function() || val.is_native_fn() || val.is_bound_method()) return Value("function");
    if (val.is_array()) return Value("array");
    if (val.is_dict()) return Value("dict");
    if (val.is_class()) return Value("class");
    if (val.is_instance()) return Value("instance");
    
    return Value("unknown");
}

static Value stdlib_random(const std::vector<Value>& args)
{
    static std::mt19937 rng(std::random_device{}());
    
    if (args.empty())
    {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return Value(dist(rng));
    }
    
    if (args.size() == 2 && args[0].is_int() && args[1].is_int())
    {
        std::uniform_int_distribution<int64_t> dist(args[0].as_int(), args[1].as_int());
        return Value(dist(rng));
    }
    
    return Value(Nil{}); // Should throw a runtime error in a robust implementation
}

static Value stdlib_input(const std::vector<Value>& args)
{
    if (!args.empty() && args[0].is_string())
    {
        std::cout << args[0].as_string();
    }
    
    std::string line;
    std::getline(std::cin, line);
    return Value(line);
}

static Value stdlib_len(const std::vector<Value>& args)
{
    if (args.empty()) return Value(0);
    
    if (args[0].is_string())
    {
        return Value(static_cast<int>(args[0].as_string().length()));
    }
    else if (args[0].is_array())
    {
        return Value(static_cast<int>(args[0].as_array()->elements.size()));
    }
    else if (args[0].is_dict())
    {
        return Value(static_cast<int>(args[0].as_dict()->elements.size()));
    }
    
    return Value(0);
}

static Value stdlib_vec2(const std::vector<Value>& args)
{
    float x = args.size() > 0 ? (float)args[0].as_number() : 0.0f;
    float y = args.size() > 1 ? (float)args[1].as_number() : 0.0f;
    return Value(ObjVec2{x, y});
}

static Value stdlib_vec3(const std::vector<Value>& args)
{
    float x = args.size() > 0 ? (float)args[0].as_number() : 0.0f;
    float y = args.size() > 1 ? (float)args[1].as_number() : 0.0f;
    float z = args.size() > 2 ? (float)args[2].as_number() : 0.0f;
    return Value(ObjVec3{x, y, z});
}

static Value stdlib_color(const std::vector<Value>& args)
{
    unsigned char r = args.size() > 0 ? (unsigned char)args[0].as_number() : 0;
    unsigned char g = args.size() > 1 ? (unsigned char)args[1].as_number() : 0;
    unsigned char b = args.size() > 2 ? (unsigned char)args[2].as_number() : 0;
    unsigned char a = args.size() > 3 ? (unsigned char)args[3].as_number() : 255;
    return Value(ObjColor{r, g, b, a});
}

static Value stdlib_sin(const std::vector<Value>& args)
{
    if (args.empty() || (!args[0].is_number())) return Value(0.0);
    return Value(std::sin(args[0].as_number()));
}

static Value stdlib_cos(const std::vector<Value>& args)
{
    if (args.empty() || (!args[0].is_number())) return Value(0.0);
    return Value(std::cos(args[0].as_number()));
}

static Value stdlib_tan(const std::vector<Value>& args)
{
    if (args.empty() || (!args[0].is_number())) return Value(0.0);
    return Value(std::tan(args[0].as_number()));
}

static Value stdlib_sqrt(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_number()) return Value(0.0);
    return Value(std::sqrt(args[0].as_number()));
}

static Value stdlib_abs(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_number()) return Value(0.0);
    return Value(std::abs(args[0].as_number()));
}

static Value stdlib_pow(const std::vector<Value>& args)
{
    if (args.size() < 2 || !args[0].is_number() || !args[1].is_number()) return Value(0.0);
    return Value(std::pow(args[0].as_number(), args[1].as_number()));
}

static Value stdlib_dot(const std::vector<Value>& args)
{
    if (args.size() < 2) return Value(0.0);
    if (args[0].is_vec3() && args[1].is_vec3())
    {
        return Value((double)(args[0].as_vec3().x * args[1].as_vec3().x + 
                              args[0].as_vec3().y * args[1].as_vec3().y + 
                              args[0].as_vec3().z * args[1].as_vec3().z));
    }
    if (args[0].is_vec2() && args[1].is_vec2())
    {
        return Value((double)(args[0].as_vec2().x * args[1].as_vec2().x + 
                              args[0].as_vec2().y * args[1].as_vec2().y));
    }
    return Value(0.0);
}

static Value stdlib_read_text(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_string()) return Value("");
    std::ifstream file(args[0].as_string());
    if (!file.is_open()) return Value("");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return Value(buffer.str());
}

static Value stdlib_write_text(const std::vector<Value>& args)
{
    if (args.size() < 2 || !args[0].is_string() || !args[1].is_string()) return Value(false);
    std::ofstream file(args[0].as_string());
    if (!file.is_open()) return Value(false);
    file << args[1].as_string();
    return Value(true);
}

// ---------------- FFI TEST ----------------
struct FakePhysicsBody {
    float x = 100.0f;
    float y = 50.0f;
};
static FakePhysicsBody g_fake_body;

static Value stdlib_get_fake_body(const std::vector<Value>& args)
{
    (void)args;
    auto ud = std::make_shared<ObjUserData>();
    ud->data = &g_fake_body;
    return Value(ud);
}

static Value stdlib_move_fake_body(const std::vector<Value>& args)
{
    if (args.size() < 3 || !args[0].is_user_data() || !args[1].is_number() || !args[2].is_number()) return Value(false);
    auto ud = args[0].as_user_data();
    FakePhysicsBody* body = static_cast<FakePhysicsBody*>(ud->data);
    body->x += static_cast<float>(args[1].as_number());
    body->y += static_cast<float>(args[2].as_number());
    return Value(true);
}

static Value stdlib_get_fake_body_x(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_user_data()) return Value(0.0);
    FakePhysicsBody* body = static_cast<FakePhysicsBody*>(args[0].as_user_data()->data);
    return Value(body->x);
}
// ------------------------------------------

// ---------------- RAYLIB BINDINGS ----------------
static Value stdlib_init_window(const std::vector<Value>& args)
{
    if (args.size() < 3) return Value(false);
    int width = (int)args[0].as_number();
    int height = (int)args[1].as_number();
    std::string title = args[2].as_string();
    InitWindow(width, height, title.c_str());
    return Value(true);
}

static Value stdlib_close_window(const std::vector<Value>& args)
{
    (void)args;
    CloseWindow();
    return Value(Nil{});
}

static Value stdlib_window_should_close(const std::vector<Value>& args)
{
    (void)args;
    return Value((bool)WindowShouldClose());
}

static Value stdlib_begin_drawing(const std::vector<Value>& args)
{
    (void)args;
    BeginDrawing();
    return Value(Nil{});
}

static Value stdlib_end_drawing(const std::vector<Value>& args)
{
    (void)args;
    EndDrawing();
    return Value(Nil{});
}

static Value stdlib_clear_background(const std::vector<Value>& args)
{
    if (args.size() < 1 || !args[0].is_color()) return Value(false);
    ObjColor c = args[0].as_color();
    ClearBackground({ c.r, c.g, c.b, c.a });
    return Value(true);
}

static Value stdlib_create_camera_3d(const std::vector<Value>& args)
{
    if (args.size() < 5) return Value(Nil{});
    Camera3D* cam = new Camera3D();
    ObjVec3 pos = args[0].as_vec3();
    ObjVec3 tgt = args[1].as_vec3();
    ObjVec3 up = args[2].as_vec3();
    cam->position = { pos.x, pos.y, pos.z };
    cam->target = { tgt.x, tgt.y, tgt.z };
    cam->up = { up.x, up.y, up.z };
    cam->fovy = (float)args[3].as_number();
    cam->projection = (int)args[4].as_number();
    auto ud = std::make_shared<ObjUserData>();
    ud->data = cam; // We leak this, but it's okay for a script engine or we can manage it later
    return Value(ud);
}

static Value stdlib_begin_mode_3d(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_user_data()) return Value(false);
    Camera3D* cam = static_cast<Camera3D*>(args[0].as_user_data()->data);
    BeginMode3D(*cam);
    return Value(true);
}

static Value stdlib_end_mode_3d(const std::vector<Value>& args)
{
    (void)args;
    EndMode3D();
    return Value(Nil{});
}

static Value stdlib_update_camera(const std::vector<Value>& args)
{
    if (args.size() < 2 || !args[0].is_user_data()) return Value(false);
    Camera3D* cam = static_cast<Camera3D*>(args[0].as_user_data()->data);
    UpdateCamera(cam, (int)args[1].as_number());
    return Value(true);
}

static Value stdlib_draw_sphere(const std::vector<Value>& args)
{
    if (args.size() < 3 || !args[0].is_vec3() || !args[2].is_color()) return Value(false);
    ObjVec3 p = args[0].as_vec3();
    float r = (float)args[1].as_number();
    ObjColor c = args[2].as_color();
    DrawSphere({ p.x, p.y, p.z }, r, { c.r, c.g, c.b, c.a });
    return Value(true);
}

static Value stdlib_is_key_down(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_number()) return Value(false);
    return Value((bool)IsKeyDown((int)args[0].as_number()));
}

static Value stdlib_is_key_pressed(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_number()) return Value(false);
    return Value((bool)IsKeyPressed((int)args[0].as_number()));
}

static Value stdlib_disable_cursor(const std::vector<Value>& args)
{
    (void)args;
    DisableCursor();
    return Value(Nil{});
}

static Value stdlib_set_target_fps(const std::vector<Value>& args)
{
    if (args.empty() || !args[0].is_number()) return Value(false);
    SetTargetFPS((int)args[0].as_number());
    return Value(true);
}

// ---------------- NATIVE PARTICLE SYSTEM ----------------
struct NativeParticle {
    Vector3 pos;
    Vector3 vel;
    Color color;
};

struct NativeParticleSystem {
    std::vector<NativeParticle> particles;
    std::mt19937 rng{std::random_device{}()};
    
    NativeParticle spawn_particle(float G, float mass) {
        std::uniform_real_distribution<float> angle_dist(0.0f, 360.0f);
        std::uniform_real_distribution<float> r_dist(10.0f, 60.0f);
        std::uniform_real_distribution<float> y_dist(-1.0f, 1.0f);
        std::uniform_int_distribution<int> c_dist(100, 255);
        
        float theta = angle_dist(rng) * PI / 180.0f;
        float r = r_dist(rng);
        
        float px = r * std::cos(theta);
        float pz = r * std::sin(theta);
        float py = y_dist(rng);
        
        float v_mag = std::sqrt(G * mass / r);
        Vector3 dir = {-pz, 0.0f, px};
        float dir_len = std::sqrt(dir.x*dir.x + dir.z*dir.z);
        if (dir_len > 0.0f) { dir.x /= dir_len; dir.z /= dir_len; }
        
        std::uniform_real_distribution<float> v_tweak(-2.0f, 2.0f);
        Vector3 vel = {
            dir.x * v_mag + v_tweak(rng),
            y_dist(rng),
            dir.z * v_mag + v_tweak(rng)
        };
        
        Color col = { (unsigned char)c_dist(rng), (unsigned char)(c_dist(rng)/2 + 50), (unsigned char)(c_dist(rng)/4), 255 };
        return { {px, py, pz}, vel, col };
    }
};

static Value stdlib_create_particle_system(const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].is_number()) return Value(Nil{});
    int count = (int)args[0].as_number();
    float G = (float)args[1].as_number();
    float mass = (float)args[2].as_number();
    
    auto sys = new NativeParticleSystem();
    sys->particles.reserve(count);
    for (int i=0; i<count; ++i) {
        sys->particles.push_back(sys->spawn_particle(G, mass));
    }
    auto ud = std::make_shared<ObjUserData>();
    ud->data = sys;
    return Value(ud);
}

static Value stdlib_update_particle_system(const std::vector<Value>& args) {
    if (args.size() < 5 || !args[0].is_user_data()) return Value(false);
    auto sys = static_cast<NativeParticleSystem*>(args[0].as_user_data()->data);
    float mass = (float)args[1].as_number();
    float G = (float)args[2].as_number();
    float event_horizon = (float)args[3].as_number();
    float dt = (float)args[4].as_number();
    
    for (auto& p : sys->particles) {
        float dx = -p.pos.x;
        float dy = -p.pos.y;
        float dz = -p.pos.z;
        float dist_sq = dx*dx + dy*dy + dz*dz;
        float dist = std::sqrt(dist_sq);
        
        if (dist < event_horizon + 1.0f) {
            p = sys->spawn_particle(G, mass);
        } else {
            float force_mag = (G * mass) / dist_sq;
            float force_x = (dx / dist) * force_mag;
            float force_y = (dy / dist) * force_mag;
            float force_z = (dz / dist) * force_mag;
            
            p.vel.x += force_x * dt;
            p.vel.y += force_y * dt;
            p.vel.z += force_z * dt;
            
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.pos.z += p.vel.z * dt;
        }
    }
    return Value(true);
}

static Value stdlib_draw_particle_system(const std::vector<Value>& args) {
    if (args.empty() || !args[0].is_user_data()) return Value(false);
    auto sys = static_cast<NativeParticleSystem*>(args[0].as_user_data()->data);
    
    for (const auto& p : sys->particles) {
        DrawSphere(p.pos, 0.25f, p.color);
    }
    return Value(true);
}

static Value stdlib_add_particles(const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].is_user_data()) return Value(false);
    auto sys = static_cast<NativeParticleSystem*>(args[0].as_user_data()->data);
    int count = (int)args[1].as_number();
    float G = (float)args[2].as_number();
    float mass = (float)args[3].as_number();
    
    for (int i = 0; i < count; ++i) {
        sys->particles.push_back(sys->spawn_particle(G, mass));
    }
    return Value(true);
}

static Value stdlib_remove_particles(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].is_user_data()) return Value(false);
    auto sys = static_cast<NativeParticleSystem*>(args[0].as_user_data()->data);
    int count = (int)args[1].as_number();
    
    for (int i = 0; i < count && !sys->particles.empty(); ++i) {
        sys->particles.pop_back();
    }
    return Value(true);
}
// ------------------------------------------------

void register_stdlib(VM& vm)
{
    vm.define_native("print", stdlib_print);
    vm.define_native("clock", stdlib_clock);
    vm.define_native("type_of", stdlib_typeof);
    vm.define_native("random", stdlib_random);
    vm.define_native("input", stdlib_input);
    vm.define_native("len", stdlib_len);
    vm.define_native("vec2", stdlib_vec2);
    vm.define_native("vec3", stdlib_vec3);
    vm.define_native("color", stdlib_color);
    
    // Math
    vm.define_native("sin", stdlib_sin);
    vm.define_native("cos", stdlib_cos);
    vm.define_native("tan", stdlib_tan);
    vm.define_native("sqrt", stdlib_sqrt);
    vm.define_native("abs", stdlib_abs);
    vm.define_native("pow", stdlib_pow);
    vm.define_native("dot", stdlib_dot);
    
    // File I/O
    vm.define_native("read_text", stdlib_read_text);
    vm.define_native("write_text", stdlib_write_text);
    
    // FFI Test
    vm.define_native("get_fake_body", stdlib_get_fake_body);
    vm.define_native("move_fake_body", stdlib_move_fake_body);
    vm.define_native("get_fake_body_x", stdlib_get_fake_body_x);

    // Arrays
    vm.define_native("array_push", stdlib_array_push);
    vm.define_native("array_pop", stdlib_array_pop);

    // Raylib
    vm.define_native("init_window", stdlib_init_window);
    vm.define_native("close_window", stdlib_close_window);
    vm.define_native("window_should_close", stdlib_window_should_close);
    vm.define_native("begin_drawing", stdlib_begin_drawing);
    vm.define_native("end_drawing", stdlib_end_drawing);
    vm.define_native("clear_background", stdlib_clear_background);
    vm.define_native("create_camera_3d", stdlib_create_camera_3d);
    vm.define_native("begin_mode_3d", stdlib_begin_mode_3d);
    vm.define_native("end_mode_3d", stdlib_end_mode_3d);
    vm.define_native("update_camera", stdlib_update_camera);
    vm.define_native("draw_sphere", stdlib_draw_sphere);
    vm.define_native("is_key_down", stdlib_is_key_down);
    vm.define_native("is_key_pressed", stdlib_is_key_pressed);
    vm.define_native("set_target_fps", stdlib_set_target_fps);
    vm.define_native("disable_cursor", stdlib_disable_cursor);

    // Native Particle System
    vm.define_native("create_particle_system", stdlib_create_particle_system);
    vm.define_native("update_particle_system", stdlib_update_particle_system);
    vm.define_native("draw_particle_system", stdlib_draw_particle_system);
    vm.define_native("add_particles", stdlib_add_particles);
    vm.define_native("remove_particles", stdlib_remove_particles);

    // Raylib constants
    vm.define_native("KEY_UP", [](const std::vector<Value>&){ return Value(265); });
    vm.define_native("KEY_DOWN", [](const std::vector<Value>&){ return Value(264); });
    vm.define_native("CAMERA_FREE", [](const std::vector<Value>&){ return Value(4); }); // CAMERA_FREE enum from raylib
    vm.define_native("CAMERA_PERSPECTIVE", [](const std::vector<Value>&){ return Value(0); });
}

} // namespace blades
