# Program Structure

This is an overview of how the shader editor program is structured.

## Overview

The shader editor is a live GLSL shader development environment with a 2D fractal viewer. It renders a full-screen quad with customizable fragment shaders, allowing real-time editing via an integrated ImGui-based editor.

The program consists of a main application window that displays the shader output, with an overlay editor panel where you can modify fragment shader code and see results immediately upon compilation.

## Architecture

The codebase follows a modular architecture with clear separation of concerns:

```
src/
├── app/         # Main application orchestration
├── camera/      # 2D camera state and navigation
├── compiler/    # Shader preprocessing and compilation
│   └── preprocessor/
└── gui/         # ImGui-based shader editor
```

### Components

#### `app` - Application Coordinator

The `App` class is the central coordinator that owns and manages all subsystems:

- **Window and OpenGL Context**: Creates GLFW window, initializes GLAD
- **Shader Program Lifecycle**: Loads, compiles, and links shaders; handles hot-reload
- **Geometry**: Manages the full-screen quad VAO/VBO
- **Input Routing**: Receives GLFW callbacks and routes to Camera or Gui
- **Subsystems**: Owns `Gui`, `Camera`, and `Compiler` instances

Key design principle: `App` eliminates global state by storing all previous global variables as member fields (input flags, mouse state, window dimensions).

#### `camera` - 2D Navigation

`Camera` is a headless 2D navigation module:
- `float _center_x, _center_y`: World-space point the camera is centered on
- `float _scale`: Abstract-to-apparent ratio for coordinate transformation
- `float _zoom_factor`: Zoom level with min/max constraints

Operations include panning, zooming at a point, and converting between screen and world coordinates. The camera is independent of GLFW and ImGui.

#### `compiler` - Shader Build Pipeline

`Compiler` wraps the shader compilation process:
1. Preprocesses source files to expand `#include` directives
2. Compiles the processed source with OpenGL
3. Returns either a shader handle or an error message

This enables splitting shaders across multiple reusable GLSL files.

#### `preprocessor` - Include Expansion

`Preprocessor` handles `#include "file.glsl"` resolution:
- Searches first in the including file's directory
- Then searches configured include directories (e.g., `shaders/`)
- Detects recursive include cycles via expansion stack

#### `gui` - Shader Editor UI

`Gui` manages the Dear ImGui interface:
- Loads fragment shader source from disk
- Presents source in a multiline text editor
- Saves edited text to temporary file on compile
- Invokes compiler on the temporary file
- Displays camera state and compilation errors in status panel
- Reports successful compilations for hot-reload

## Execution Flow

### Startup Sequence

1. `main.cpp` creates `App` and calls `initialize()`:
   - Initialize GLFW and GLAD
   - Create OpenGL 3.3 core context
   - Set up input callbacks
   - Initialize full-screen quad geometry
   - Construct `Compiler`, `Camera`, and `Gui`
   - Load and compile initial shaders
   - Link vertex and fragment shaders into program
   - Load fragment shader source into editor

2. `run()` enters the main render loop

### Frame Execution

Each frame, `App::run()` executes:

1. **Poll Events** - GLFW processes window events
2. **Process Input** - Update camera movement if GUI doesn't capture input
3. **Update Uniforms** - Push `uCenter`, `uScale`, `uAspect`, `uTime` to shader
4. **Render Scene** - Draw full-screen quad with current shader program
5. **Render GUI** - Draw ImGui editor panels
6. **Hot Reload** - Check if GUI produced new shader, link and activate it
7. **Swap Buffers** - Present the frame

### Hot Reload Flow

```
User clicks "Compile and Run" in Gui
    ↓
Gui::tryCompileShader():
    - Saves editor content to temporary file
    - Calls compiler->compile() → gets GLuint fragment shader
    - Stores: hasNewShader_ = true, compiledShader_ = id
    ↓
App::try_hot_reload() (each frame):
    - Detects hasNewShader_ flag
    - Links new fragment shader with existing vertex shader
    - On success: replaces active program, deletes old program
    - On failure: keeps old program, logs error
    - Cleans up fragment shader object
```

Key principle: `App` owns all OpenGL program objects. `Gui` produces individual shaders, `App` manages their lifecycle.

## Input Handling

Input flows through GLFW callbacks registered by `App`:

```
GLFW Event → App::*_callback (static) → App::on_* (instance)
    ↓
Check Gui::wantsKeyboardInput() / wantsMouseInput()
    ↓
Route to Camera OR Gui
```

- **Keyboard (arrows, O/N)**: Camera movement when GUI not focused
- **Mouse drag**: Camera panning when not over GUI
- **Mouse scroll**: Zoom toward cursor position when not over GUI

This design keeps `Camera` independent of windowing system details.

## Shader Compilation Pipeline

### Stage 1: Preprocessing

`Preprocessor::process_source()` recursively expands includes:

```glsl
// user_shader.glsl
#include "lib.glsl"
void main() { ... }

// Becomes processed source:
// [contents of lib.glsl]
// void main() { ... }
```

### Stage 2: Compilation

`Compiler::compile()` sends preprocessed source to OpenGL:
- Success: returns compiled shader object handle
- Failure: returns error log string

### Stage 3: Linking

`App::link_shader_program()` attaches vertex and fragment shaders:
- Success: returns linked program ID
- Failure: returns 0, logs linker error

The vertex shader is loaded once at startup and reused for hot-reloads. Only the fragment shader is recompiled during editing.

## Data Flow

### Uniform Updates (per frame)

```
Camera state → Shader uniforms
    _center_x/y  → uCenter (vec2)
    _zoom_factor → uScale (float, with _scale)
    window size  → uAspect (float)
    glfwGetTime  → uTime (float)
```

### Coordinate Transformation

The shader transforms from normalized device coordinates (NDC) to world space:

```glsl
world_x = ndc_x * uScale * aspect + uCenter.x
world_y = ndc_y * uScale + uCenter.y
```

This allows the fragment shader to work in abstract mathematical coordinates regardless of window size or zoom level.

## Resource Ownership

Resource cleanup order (important for OpenGL context):

1. `Gui::shutdown()` - ImGui cleanup while GL context exists
2. Delete OpenGL objects (program, shaders, VAO, VBO)
3. Destroy GLFW window
4. `glfwTerminate()`
5. Destroy subsystems (`_gui`, `_camera`, `_compiler`)

This order ensures ImGui can clean up its GL resources before the context is destroyed.

## Naming Conventions

- **Private members**: `_prefix` (e.g., `_window`, `_camera`)
- **Classes**: PascalCase (e.g., `App`, `Gui`, `Camera`)
- **Methods**: `snake_case` (e.g., `initialize()`, `try_hot_reload()`)
- **Constants**: `SCREAMING_SNAKE_CASE` in headers, `kCamelCase` or local `snake_case` in implementation
