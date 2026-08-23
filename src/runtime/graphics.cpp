#include "runtime/graphics.hpp"
#include "backend/vm.hpp"

#include <raylib.h>
#include <iostream>

namespace blades
{

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

static Value blades_begin_mode_3d(const std::vector<Value>& args)
{
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
    
    vm.define_native("begin_drawing", blades_begin_drawing);
    vm.define_native("end_drawing", blades_end_drawing);
    vm.define_native("clear_background", blades_clear_background);
    
    vm.define_native("draw_circle", blades_draw_circle);
    vm.define_native("draw_rectangle", blades_draw_rectangle);
    
    vm.define_native("begin_mode_3d", blades_begin_mode_3d);
    vm.define_native("end_mode_3d", blades_end_mode_3d);
    vm.define_native("draw_cube", blades_draw_cube);
    vm.define_native("draw_line_3d", blades_draw_line_3d);
}

} // namespace blades
