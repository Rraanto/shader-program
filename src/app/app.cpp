/*
 * App.cpp
 *
 * Implementation of the App class managing the shader editor application.
 */

#include "app.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#include "camera/camera.h"
#include "compiler/compiler/compiler.h"
#include "gui/gui.h"

App::App() = default;

App::~App() { shutdown(); }

bool App::initialize(int width, int height, const char *title) {
    _window_width = width;
    _window_height = height;

    // Initialize GLFW
    if (!init_glfw()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    _window = glfwCreateWindow(_window_width, _window_height, title, nullptr,
                               nullptr);
    if (!_window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(_window);

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(_window, this);

    // Initialize OpenGL
    if (!init_opengl()) {
        std::cerr << "Failed to initialize OpenGL\n";
        glfwDestroyWindow(_window);
        glfwTerminate();
        return false;
    }

    // Setup callbacks
    glfwSetKeyCallback(_window, key_callback);
    glfwSetMouseButtonCallback(_window, mouse_button_callback);
    glfwSetCursorPosCallback(_window, cursor_pos_callback);
    glfwSetScrollCallback(_window, scroll_callback);

    // Initialize geometry
    if (!init_geometry()) {
        std::cerr << "Failed to initialize geometry\n";
        glfwDestroyWindow(_window);
        glfwTerminate();
        return false;
    }

    // Initialize camera
    init_camera();

    // Initialize compiler
    std::vector<std::filesystem::path> include_dirs = {
        std::filesystem::path(SHADERS_DIR)};
    _compiler = std::make_unique<Compiler>(include_dirs);

    // Initialize shaders
    _vertex_shader_path =
        (std::filesystem::path(SHADERS_DIR) / "vertex_shader.glsl").string();
    _fragment_shader_path =
        (std::filesystem::path(SHADERS_DIR) / "fragment_shader.glsl").string();

    if (!init_shaders()) {
        std::cerr << "Failed to initialize shaders\n";
        cleanup_geometry();
        glfwDestroyWindow(_window);
        glfwTerminate();
        return false;
    }

    // Initialize GUI
    if (!init_gui()) {
        std::cerr << "Failed to initialize GUI\n";
        cleanup_shader_program();
        cleanup_geometry();
        glfwDestroyWindow(_window);
        glfwTerminate();
        return false;
    }

    return true;
}

void App::run() {
    if (!_window) {
        std::cerr << "App not initialized\n";
        return;
    }

    _running = true;

    while (!glfwWindowShouldClose(_window) && _running) {
        float time = static_cast<float>(glfwGetTime());

        glfwPollEvents();
        process_input();

        // Update window dimensions
        int w, h;
        glfwGetFramebufferSize(_window, &w, &h);
        _window_width = w;
        _window_height = h;

        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update shader uniforms
        update_uniforms(time);

        // Render shader viewport
        glUseProgram(_shader_program);
        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLES, 0, _vertex_count);

        // Render GUI
        render();

        // Check for hot-reload
        try_hot_reload();

        glfwSwapBuffers(_window);
    }
}

void App::shutdown() {
    if (!_window)
        return;

    _running = false;

    // Shutdown GUI first
    if (_gui) {
        _gui->shutdown();
        _gui.reset();
    }

    // Cleanup OpenGL resources
    cleanup_shader_program();
    cleanup_geometry();

    // Destroy window and terminate GLFW
    glfwDestroyWindow(_window);
    _window = nullptr;
    glfwTerminate();

    // Clean up other subsystems
    _compiler.reset();
    _camera.reset();
}

bool App::init_glfw() {
    glfwSetErrorCallback(error_callback);
    return glfwInit() != 0;
}

bool App::init_opengl() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return false;
    }
    return true;
}

bool App::init_shaders() {
    // Compile vertex shader from file or source
    Compiler::CompileOutput vertex_result =
        _compiler->compile(_vertex_shader_path, GL_VERTEX_SHADER);

    if (!vertex_result.success) {
        std::cerr << "Vertex shader compilation failed:\n"
                  << vertex_result.error << std::endl;
        return false;
    }
    _vertex_shader = vertex_result.shader;

    // Compile fragment shader
    Compiler::CompileOutput fragment_result =
        _compiler->compile(_fragment_shader_path, GL_FRAGMENT_SHADER, true);

    if (!fragment_result.success) {
        std::cerr << "Fragment shader compilation failed:\n"
                  << fragment_result.error << std::endl;
        glDeleteShader(_vertex_shader);
        _vertex_shader = 0;
        return false;
    }

    // Link shader program
    unsigned int program = 0;
    if (!link_shader_program(fragment_result.shader, program)) {
        glDeleteShader(fragment_result.shader);
        glDeleteShader(_vertex_shader);
        _vertex_shader = 0;
        return false;
    }

    // Clean up fragment shader (vertex shader kept for hot-reload)
    glDeleteShader(fragment_result.shader);

    _shader_program = program;
    return true;
}

bool App::init_geometry() {
    // Full-screen quad (two triangles)
    float vertices[] = {
        -1.0f, -1.0f,  // bottom left
        1.0f,  -1.0f,  // bottom right
        1.0f,  1.0f,   // top right

        -1.0f, -1.0f,  // bottom left
        1.0f,  1.0f,   // top right
        -1.0f, 1.0f    // top left
    };
    const int dimension = 2;
    _vertex_count = sizeof(vertices) / (dimension * sizeof(float));

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, dimension, GL_FLOAT, GL_FALSE,
                          dimension * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    return true;
}

bool App::init_gui() {
    _gui = std::make_unique<Gui>();
    if (!_gui->initialize(_window)) {
        return false;
    }

    _gui->setCompiler(_compiler.get());
    _gui->setCamera(_camera.get());
    _gui->setShaderPath(_fragment_shader_path);

    // Load initial shader into editor
    if (!_gui->loadShaderFromFile(_fragment_shader_path)) {
        std::cerr << "Warning: Could not load shader into editor\n";
    }

    return true;
}

void App::init_camera() {
    // Seahorse valley interesting point
    float _p_x = -0.214268f;
    float _p_y = 0.652873f;
    _camera = std::make_unique<Camera>(_p_x, _p_y);
    _camera->zoom_in(0.8f);
}

void App::process_input() {
    if (!_gui || !_camera)
        return;

    // Only process camera movement if GUI doesn't want keyboard
    if (!_gui->wantsKeyboardInput()) {
        float right = _keys._move_right ? _camera_stride : 0.0f;
        float left = _keys._move_left ? -_camera_stride : 0.0f;
        float up = _keys._move_up ? _camera_stride : 0.0f;
        float down = _keys._move_down ? -_camera_stride : 0.0f;
        _camera->move(right + left, up + down);

        _camera->zoom_in(_keys._zoom_in ? _zoom_stride : 0.0f);
        _camera->zoom_out(_keys._zoom_out ? _zoom_stride : 0.0f);
    }
}

void App::update_uniforms(float time) {
    if (!_camera)
        return;

    GLint loc_center = glGetUniformLocation(_shader_program, "uCenter");
    GLint loc_scale = glGetUniformLocation(_shader_program, "uScale");
    GLint loc_aspect = glGetUniformLocation(_shader_program, "uAspect");
    GLint loc_time = glGetUniformLocation(_shader_program, "uTime");

    if (loc_center >= 0)
        glUniform2f(loc_center, _camera->get_x(), _camera->get_y());
    if (loc_scale >= 0)
        glUniform1f(loc_scale, _camera->get_zoom() * _camera->get_scale());
    if (loc_aspect >= 0)
        glUniform1f(loc_aspect,
                    static_cast<float>(_window_width) /
                        static_cast<float>(_window_height));
    if (loc_time >= 0)
        glUniform1f(loc_time, time);
}

void App::render() {
    if (!_gui)
        return;

    _gui->beginFrame();
    _gui->render();
    _gui->endFrame();
}

void App::try_hot_reload() {
    if (!_gui || !_compiler)
        return;

    if (!_gui->hasNewShader())
        return;

    unsigned int new_fragment = _gui->getCompiledShader();
    if (new_fragment == 0)
        return;

    // Try to link with existing vertex shader
    unsigned int new_program = 0;
    if (!link_shader_program(new_fragment, new_program)) {
        // Link failed - clean up and keep old program
        glDeleteShader(new_fragment);
        _gui->clearNewShaderFlag();
        return;
    }

    // Success - switch to new program
    glDeleteProgram(_shader_program);
    _shader_program = new_program;
    glUseProgram(_shader_program);
    glBindVertexArray(_vao);

    glDeleteShader(new_fragment);
    _gui->clearNewShaderFlag();
}

bool App::link_shader_program(unsigned int fragment_shader,
                               unsigned int &out_program) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, _vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int success = 0;
    char info_log[512] = {0};
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << info_log << "\n";
        glDeleteProgram(program);
        return false;
    }

    out_program = program;
    return true;
}

void App::cleanup_shader_program() {
    if (_shader_program) {
        glDeleteProgram(_shader_program);
        _shader_program = 0;
    }
    if (_vertex_shader) {
        glDeleteShader(_vertex_shader);
        _vertex_shader = 0;
    }
}

void App::cleanup_geometry() {
    if (_vao) {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
    if (_vbo) {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }
}

// GLFW callbacks - static trampolines
void App::error_callback(int error, const char *description) {
    std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

void App::key_callback(GLFWwindow *window, int key, int scancode, int action,
                       int mods) {
    App *app = static_cast<App *>(glfwGetWindowUserPointer(window));
    if (app) {
        app->on_key(key, scancode, action, mods);
    }
}

void App::mouse_button_callback(GLFWwindow *window, int button, int action,
                                int mods) {
    App *app = static_cast<App *>(glfwGetWindowUserPointer(window));
    if (app) {
        app->on_mouse_button(button, action, mods);
    }
}

void App::cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    App *app = static_cast<App *>(glfwGetWindowUserPointer(window));
    if (app) {
        app->on_cursor_pos(xpos, ypos);
    }
}

void App::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    App *app = static_cast<App *>(glfwGetWindowUserPointer(window));
    if (app) {
        app->on_scroll(xoffset, yoffset);
    }
}

// Instance callback handlers
void App::on_key(int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_RIGHT) {
        _keys._move_right = (action == GLFW_PRESS);
        if (action == GLFW_RELEASE)
            _keys._move_right = false;
    }
    if (key == GLFW_KEY_LEFT) {
        _keys._move_left = (action == GLFW_PRESS);
        if (action == GLFW_RELEASE)
            _keys._move_left = false;
    }
    if (key == GLFW_KEY_UP) {
        _keys._move_up = (action == GLFW_PRESS);
        if (action == GLFW_RELEASE)
            _keys._move_up = false;
    }
    if (key == GLFW_KEY_DOWN) {
        _keys._move_down = (action == GLFW_PRESS);
        if (action == GLFW_RELEASE)
            _keys._move_down = false;
    }
    if (key == GLFW_KEY_O) {
        _keys._zoom_in = (action == GLFW_PRESS);
        if (action == GLFW_RELEASE)
            _keys._zoom_in = false;
    }
    if (key == GLFW_KEY_N) {
        _keys._zoom_out = (action == GLFW_PRESS);
        if (action == GLFW_RELEASE)
            _keys._zoom_out = false;
    }
}

void App::on_mouse_button(int button, int action, int mods) {
    (void)mods;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Only start drag if not clicking on GUI
            if (_gui && !_gui->wantsMouseInput()) {
                _mouse._is_dragging = true;
                glfwGetCursorPos(_window, &_mouse._last_x, &_mouse._last_y);
            }
        } else if (action == GLFW_RELEASE) {
            _mouse._is_dragging = false;
        }
    }
}

void App::on_cursor_pos(double xpos, double ypos) {
    if (!_mouse._is_dragging || !_camera)
        return;

    double dx = xpos - _mouse._last_x;
    double dy = ypos - _mouse._last_y;
    _mouse._last_x = xpos;
    _mouse._last_y = ypos;

    if (dx == 0.0 && dy == 0.0)
        return;

    if (_window_width <= 0 || _window_height <= 0)
        return;

    const float ndc_dx = static_cast<float>(2.0 * dx / _window_width);
    const float ndc_dy = static_cast<float>(-2.0 * dy / _window_height);
    const float aspect = static_cast<float>(_window_width) /
                         static_cast<float>(_window_height);
    const float scale = _camera->get_scale();

    // Dragging moves the world with the cursor, so camera moves opposite
    _camera->move(-ndc_dx * aspect * scale, -ndc_dy * scale);
}

void App::on_scroll(double xoffset, double yoffset) {
    (void)xoffset;

    if (!_camera || !_gui)
        return;

    // Don't zoom if GUI wants mouse
    if (_gui->wantsMouseInput())
        return;

    // Get mouse position in normalized device coordinates
    double mouse_x, mouse_y;
    glfwGetCursorPos(_window, &mouse_x, &mouse_y);

    // Convert to world coordinates before zoom
    float ndc_x =
        (static_cast<float>(mouse_x) / static_cast<float>(_window_width)) *
            2.0f -
        1.0f;
    float ndc_y =
        1.0f -
        (static_cast<float>(mouse_y) / static_cast<float>(_window_height)) *
            2.0f;

    // Convert NDC to world coordinates
    float aspect = static_cast<float>(_window_width) /
                   static_cast<float>(_window_height);
    float world_x = ndc_x * _camera->get_scale() * _camera->get_zoom() * aspect +
                    _camera->get_x();
    float world_y = ndc_y * _camera->get_scale() * _camera->get_zoom() +
                    _camera->get_y();

    // Zoom amount
    float zoom_amount = static_cast<float>(yoffset) * 0.1f;
    _camera->zoom_at(zoom_amount, world_x, world_y);
}
