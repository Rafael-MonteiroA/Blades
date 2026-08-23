#include "runtime/graphics.hpp"
#include "backend/vm.hpp"

#include <raylib.h>
#include <iostream>
#include <cmath>
#include <map>

namespace blades
{

static int string_to_key(const std::string& key_name) {
    if (key_name == "SPACE") return KEY_SPACE;
    if (key_name == "ESCAPE") return KEY_ESCAPE;
    if (key_name == "ENTER") return KEY_ENTER;
    if (key_name == "RIGHT") return KEY_RIGHT;
    if (key_name == "LEFT") return KEY_LEFT;
    if (key_name == "DOWN") return KEY_DOWN;
    if (key_name == "UP") return KEY_UP;
    if (key_name == "W") return KEY_W;
    if (key_name == "A") return KEY_A;
    if (key_name == "S") return KEY_S;
    if (key_name == "D") return KEY_D;
    return 0;
}

static int string_to_mouse_button(const std::string& btn_name) {
    if (btn_name == "LEFT") return MOUSE_BUTTON_LEFT;
    if (btn_name == "RIGHT") return MOUSE_BUTTON_RIGHT;
    if (btn_name == "MIDDLE") return MOUSE_BUTTON_MIDDLE;
    return 0;
}

// ── Texturas ─────────────────────────────────────────────────────────────────
static std::map<std::string, Texture2D> loaded_textures;

static Value blades_load_texture(const std::vector<Value>& args) {
    if (args.size() != 1 || !args[0].is_string()) return Value(Nil{});
    std::string filename = args[0].as_string();
    if (loaded_textures.find(filename) == loaded_textures.end()) {
        loaded_textures[filename] = LoadTexture(filename.c_str());
    }
    return Value(Nil{});
}


// ── Audio ────────────────────────────────────────────────────────────────────
static std::map<std::string, Sound> loaded_sounds;

static Value blades_init_audio(const std::vector<Value>& args) {
    (void)args;
    InitAudioDevice();
    return Value(Nil{});
}

static Value blades_play_sound(const std::vector<Value>& args) {
    if (args.size() != 1 || !args[0].is_string()) return Value(Nil{});
    std::string filename = args[0].as_string();
    
    if (loaded_sounds.find(filename) == loaded_sounds.end()) {
        loaded_sounds[filename] = LoadSound(filename.c_str());
    }
    
    PlaySound(loaded_sounds[filename]);
    return Value(Nil{});
}

// ── Janela ───────────────────────────────────────────────────────────────────

static Value blades_window_init(const std::vector<Value>& args)
{
    if (args.size() != 3) {
        std::cerr << "Runtime Error: window_init expected 3 arguments.\n";
        return Value(Nil{});
    }
    
    int width = args[0].is_number() ? args[0].as_int() : 800;
    int height = args[1].is_number() ? args[1].as_int() : 600;
    std::string title = args[2].is_string() ? args[2].as_string() : "Blades";
    
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
    return Value(Nil{});
}

static Value blades_window_should_close(const std::vector<Value>& args)
{
    (void)args;
    return Value(WindowShouldClose());
}

static Value blades_window_close(const std::vector<Value>& args)
{
    (void)args;
    CloseWindow();
    return Value(Nil{});
}

// ── Input & Tempo ────────────────────────────────────────────────────────────

static Value blades_is_key_down(const std::vector<Value>& args)
{
    if (args.size() != 1 || !args[0].is_string()) {
        std::cerr << "Runtime Error: is_key_down expected 1 string argument.\n";
        return Value(false);
    }
    int key = string_to_key(args[0].as_string());
    return Value(IsKeyDown(key));
}

static Value blades_is_key_pressed(const std::vector<Value>& args)
{
    if (args.size() != 1 || !args[0].is_string()) {
        std::cerr << "Runtime Error: is_key_pressed expected 1 string argument.\n";
        return Value(false);
    }
    int key = string_to_key(args[0].as_string());
    return Value(IsKeyPressed(key));
}

static Value blades_get_frame_time(const std::vector<Value>& args)
{
    (void)args;
    return Value(static_cast<double>(GetFrameTime()));
}

static Value blades_disable_cursor(const std::vector<Value>& args) {
    (void)args;
    DisableCursor();
    return Value(Nil{});
}

static Value blades_enable_cursor(const std::vector<Value>& args) {
    (void)args;
    EnableCursor();
    return Value(Nil{});
}

static Value blades_is_mouse_button_pressed(const std::vector<Value>& args) {
    if (args.size() != 1 || !args[0].is_string()) return Value(false);
    return Value(IsMouseButtonPressed(string_to_mouse_button(args[0].as_string())));
}

static Value blades_distance_3d(const std::vector<Value>& args) {
    if (args.size() != 6) return Value(0.0);
    double dx = args[0].as_number() - args[3].as_number();
    double dy = args[1].as_number() - args[4].as_number();
    double dz = args[2].as_number() - args[5].as_number();
    return Value(std::sqrt(dx*dx + dy*dy + dz*dz));
}

// ── Desenho Básico ───────────────────────────────────────────────────────────

static Value blades_begin_drawing(const std::vector<Value>& args)
{
    (void)args;
    BeginDrawing();
    return Value(Nil{});
}

static Value blades_end_drawing(const std::vector<Value>& args)
{
    (void)args;
    EndDrawing();
    return Value(Nil{});
}

static Value blades_clear_background(const std::vector<Value>& args)
{
    if (args.size() != 3) {
        std::cerr << "Runtime Error: clear_background expected 3 arguments (r, g, b).\n";
        return Value(Nil{});
    }
    
    unsigned char r = static_cast<unsigned char>(args[0].as_int());
    unsigned char g = static_cast<unsigned char>(args[1].as_int());
    unsigned char b = static_cast<unsigned char>(args[2].as_int());
    
    ClearBackground({r, g, b, 255});
    return Value(Nil{});
}

// ── Desenho 2D ───────────────────────────────────────────────────────────────

static Value blades_draw_text(const std::vector<Value>& args)
{
    if (args.size() != 7) {
        std::cerr << "Runtime Error: draw_text expected 7 arguments (text, x, y, size, r, g, b).\n";
        return Value(Nil{});
    }
    std::string text = args[0].is_string() ? args[0].as_string() : "null";
    int x = args[1].as_int();
    int y = args[2].as_int();
    int size = args[3].as_int();
    unsigned char r = static_cast<unsigned char>(args[4].as_int());
    unsigned char g = static_cast<unsigned char>(args[5].as_int());
    unsigned char b = static_cast<unsigned char>(args[6].as_int());
    
    DrawText(text.c_str(), x, y, size, {r, g, b, 255});
    return Value(Nil{});
}

static Value blades_draw_circle(const std::vector<Value>& args)
{
    if (args.size() != 6) {
        std::cerr << "Runtime Error: draw_circle expected 6 arguments (x, y, radius, r, g, b).\n";
        return Value(Nil{});
    }
    
    int x = args[0].as_int();
    int y = args[1].as_int();
    float radius = static_cast<float>(args[2].as_number());
    unsigned char r = static_cast<unsigned char>(args[3].as_int());
    unsigned char g = static_cast<unsigned char>(args[4].as_int());
    unsigned char b = static_cast<unsigned char>(args[5].as_int());
    
    DrawCircle(x, y, radius, {r, g, b, 255});
    return Value(Nil{});
}

static Value blades_draw_rectangle(const std::vector<Value>& args)
{
    if (args.size() != 7) {
        std::cerr << "Runtime Error: draw_rectangle expected 7 arguments (x, y, w, h, r, g, b).\n";
        return Value(Nil{});
    }
    
    int x = args[0].as_int();
    int y = args[1].as_int();
    int w = args[2].as_int();
    int h = args[3].as_int();
    unsigned char r = static_cast<unsigned char>(args[4].as_int());
    unsigned char g = static_cast<unsigned char>(args[5].as_int());
    unsigned char b = static_cast<unsigned char>(args[6].as_int());
    
    DrawRectangle(x, y, w, h, {r, g, b, 255});
    return Value(Nil{});
}

// ── Desenho 3D ───────────────────────────────────────────────────────────────

static Camera3D camera = { 0 };

static Value blades_update_camera_first_person(const std::vector<Value>& args) {
    (void)args;
    UpdateCamera(&camera, CAMERA_FIRST_PERSON);
    return Value(Nil{});
}

static Value blades_get_camera_x(const std::vector<Value>& args) {
    (void)args; return Value(static_cast<double>(camera.position.x));
}
static Value blades_get_camera_y(const std::vector<Value>& args) {
    (void)args; return Value(static_cast<double>(camera.position.y));
}
static Value blades_get_camera_z(const std::vector<Value>& args) {
    (void)args; return Value(static_cast<double>(camera.position.z));
}

static Value blades_set_camera_position(const std::vector<Value>& args) {
    if (args.size() == 3) {
        camera.position.x = static_cast<float>(args[0].as_number());
        camera.position.y = static_cast<float>(args[1].as_number());
        camera.position.z = static_cast<float>(args[2].as_number());
    }
    return Value(Nil{});
}

static Value blades_init_camera(const std::vector<Value>& args) {
    if (args.size() == 6) {
        camera.position = { static_cast<float>(args[0].as_number()), static_cast<float>(args[1].as_number()), static_cast<float>(args[2].as_number()) };
        camera.target = { static_cast<float>(args[3].as_number()), static_cast<float>(args[4].as_number()), static_cast<float>(args[5].as_number()) };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    }
    return Value(Nil{});
}

static Value blades_begin_mode_3d(const std::vector<Value>& args)
{
    if (args.empty()) {
        BeginMode3D(camera);
        return Value(Nil{});
    }

    if (args.size() != 6) {
        std::cerr << "Runtime Error: begin_mode_3d expected 6 arguments (cam_x, cam_y, cam_z, tar_x, tar_y, tar_z).\n";
        return Value(Nil{});
    }
    
    camera.position = { 
        static_cast<float>(args[0].as_number()), 
        static_cast<float>(args[1].as_number()), 
        static_cast<float>(args[2].as_number()) 
    };
    camera.target = { 
        static_cast<float>(args[3].as_number()), 
        static_cast<float>(args[4].as_number()), 
        static_cast<float>(args[5].as_number()) 
    };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    BeginMode3D(camera);
    return Value(Nil{});
}

static Value blades_end_mode_3d(const std::vector<Value>& args)
{
    (void)args;
    EndMode3D();
    return Value(Nil{});
}

static Value blades_draw_sphere(const std::vector<Value>& args)
{
    if (args.size() != 7) {
        std::cerr << "Runtime Error: draw_sphere expected 7 arguments (x, y, z, radius, r, g, b).\n";
        return Value(Nil{});
    }
    
    Vector3 pos = {
        static_cast<float>(args[0].as_number()),
        static_cast<float>(args[1].as_number()),
        static_cast<float>(args[2].as_number())
    };
    float radius = static_cast<float>(args[3].as_number());
    unsigned char r = static_cast<unsigned char>(args[4].as_int());
    unsigned char g = static_cast<unsigned char>(args[5].as_int());
    unsigned char b = static_cast<unsigned char>(args[6].as_int());
    
    DrawSphere(pos, radius, {r, g, b, 255});
    return Value(Nil{});
}

static Value blades_draw_cube(const std::vector<Value>& args)
{
    if (args.size() != 9) {
        std::cerr << "Runtime Error: draw_cube expected 9 arguments (x, y, z, w, h, l, r, g, b).\n";
        return Value(Nil{});
    }
    
    Vector3 pos = {
        static_cast<float>(args[0].as_number()),
        static_cast<float>(args[1].as_number()),
        static_cast<float>(args[2].as_number())
    };
    float w = static_cast<float>(args[3].as_number());
    float h = static_cast<float>(args[4].as_number());
    float l = static_cast<float>(args[5].as_number());
    unsigned char r = static_cast<unsigned char>(args[6].as_int());
    unsigned char g = static_cast<unsigned char>(args[7].as_int());
    unsigned char b = static_cast<unsigned char>(args[8].as_int());
    
    DrawCube(pos, w, h, l, {r, g, b, 255});
    return Value(Nil{});
}

static Model unit_cube = { 0 };
static bool cube_model_initialized = false;

static Value blades_draw_cube_texture(const std::vector<Value>& args)
{
    if (args.size() != 10) return Value(Nil{});
    std::string tex_name = args[0].as_string();
    
    if (!cube_model_initialized) {
        unit_cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
        cube_model_initialized = true;
    }
    
    if (loaded_textures.find(tex_name) != loaded_textures.end()) {
        unit_cube.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = loaded_textures[tex_name];
        
        Vector3 pos = {
            static_cast<float>(args[1].as_number()),
            static_cast<float>(args[2].as_number()),
            static_cast<float>(args[3].as_number())
        };
        Vector3 scale = {
            static_cast<float>(args[4].as_number()),
            static_cast<float>(args[5].as_number()),
            static_cast<float>(args[6].as_number())
        };
        unsigned char r = static_cast<unsigned char>(args[7].as_int());
        unsigned char g = static_cast<unsigned char>(args[8].as_int());
        unsigned char b = static_cast<unsigned char>(args[9].as_int());
        
        DrawModelEx(unit_cube, pos, {0,1,0}, 0.0f, scale, {r, g, b, 255});
    }
    return Value(Nil{});
}

static Value blades_draw_billboard(const std::vector<Value>& args)
{
    if (args.size() != 8) return Value(Nil{});
    std::string tex_name = args[0].as_string();
    Vector3 pos = {
        static_cast<float>(args[1].as_number()),
        static_cast<float>(args[2].as_number()),
        static_cast<float>(args[3].as_number())
    };
    float size = static_cast<float>(args[4].as_number());
    unsigned char r = static_cast<unsigned char>(args[5].as_int());
    unsigned char g = static_cast<unsigned char>(args[6].as_int());
    unsigned char b = static_cast<unsigned char>(args[7].as_int());
    
    if (loaded_textures.find(tex_name) != loaded_textures.end()) {
        DrawBillboard(camera, loaded_textures[tex_name], pos, size, {r, g, b, 255});
    }
    return Value(Nil{});
}

static Value blades_draw_line_3d(const std::vector<Value>& args)
{
    if (args.size() != 9) {
        std::cerr << "Runtime Error: draw_line_3d expected 9 arguments (sx, sy, sz, ex, ey, ez, r, g, b).\n";
        return Value(Nil{});
    }
    
    Vector3 start = {
        static_cast<float>(args[0].as_number()),
        static_cast<float>(args[1].as_number()),
        static_cast<float>(args[2].as_number())
    };
    Vector3 end = {
        static_cast<float>(args[3].as_number()),
        static_cast<float>(args[4].as_number()),
        static_cast<float>(args[5].as_number())
    };
    unsigned char r = static_cast<unsigned char>(args[6].as_int());
    unsigned char g = static_cast<unsigned char>(args[7].as_int());
    unsigned char b = static_cast<unsigned char>(args[8].as_int());
    
    DrawLine3D(start, end, {r, g, b, 255});
    return Value(Nil{});
}

void register_graphics_functions(VM& vm)
{
    vm.define_native("window_init", blades_window_init);
    vm.define_native("window_should_close", blades_window_should_close);
    vm.define_native("window_close", blades_window_close);
    
    vm.define_native("is_key_down", blades_is_key_down);
    vm.define_native("is_key_pressed", blades_is_key_pressed);
    vm.define_native("get_frame_time", blades_get_frame_time);
    
    vm.define_native("disable_cursor", blades_disable_cursor);
    vm.define_native("enable_cursor", blades_enable_cursor);
    vm.define_native("is_mouse_button_pressed", blades_is_mouse_button_pressed);
    vm.define_native("distance_3d", blades_distance_3d);
    
    vm.define_native("init_audio", blades_init_audio);
    vm.define_native("play_sound", blades_play_sound);
    
    vm.define_native("begin_drawing", blades_begin_drawing);
    vm.define_native("end_drawing", blades_end_drawing);
    vm.define_native("clear_background", blades_clear_background);
    
    vm.define_native("draw_text", blades_draw_text);
    vm.define_native("draw_circle", blades_draw_circle);
    vm.define_native("draw_rectangle", blades_draw_rectangle);
    
    vm.define_native("begin_mode_3d", blades_begin_mode_3d);
    vm.define_native("end_mode_3d", blades_end_mode_3d);
    
    vm.define_native("update_camera_first_person", blades_update_camera_first_person);
    vm.define_native("get_camera_x", blades_get_camera_x);
    vm.define_native("get_camera_y", blades_get_camera_y);
    vm.define_native("get_camera_z", blades_get_camera_z);
    vm.define_native("set_camera_position", blades_set_camera_position);
    vm.define_native("init_camera", blades_init_camera);
    
    vm.define_native("draw_sphere", blades_draw_sphere);
    vm.define_native("draw_cube", blades_draw_cube);
    vm.define_native("draw_cube_texture", blades_draw_cube_texture);
    vm.define_native("draw_billboard", blades_draw_billboard);
    vm.define_native("load_texture", blades_load_texture);
    vm.define_native("draw_line_3d", blades_draw_line_3d);
}

} // namespace blades
