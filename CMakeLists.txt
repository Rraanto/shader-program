cmake_minimum_required(VERSION 3.20)

project(ShaderCompiler LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-g -ggdb3 -O0 -fno-omit-frame-pointer)

# external dependencies
include(FetchContent)

# glfw's own CMakeLists.txt does the job
# target name is glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # avoid building glfw test exes
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# Glad library
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad
  PUBLIC 
  external/glad/include 
  external/glad/KHR${GLAD_COMPAT_INCLUDE_DIR}
  PRIVATE
  external/glad/include/glad
)

# ImGui library
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.0  # Stable release
)
FetchContent_MakeAvailable(imgui)

# ImGui doesn't have a CMakeLists.txt, so we create the target manually
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui
  PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PRIVATE glfw glad)

add_library(camera STATIC src/camera/camera.cpp)
target_include_directories(camera
  PUBLIC
  src
  PRIVATE
  src/camera
)

# compiler library
add_library(compiler STATIC
  src/compiler/compiler/compiler.cpp
  src/compiler/preprocessor/preprocessor.cpp
)
target_include_directories(compiler PUBLIC src/compiler)
target_link_libraries(compiler PRIVATE glad)

# GUI library
add_library(gui STATIC
  src/gui/gui.cpp
)
target_include_directories(compiler PUBLIC src)
target_include_directories(gui PUBLIC src/gui src/compiler src)
target_link_libraries(gui PRIVATE glad glfw imgui compiler camera)

# App library
add_library(app STATIC
  src/app/app.cpp
)
target_include_directories(app PUBLIC src)
target_link_libraries(app PRIVATE glad glfw imgui gui camera compiler)
target_compile_definitions(app PRIVATE SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

# Main executable
add_executable(main src/main.cpp)
target_link_libraries(main
  PRIVATE
  app
  camera
  glfw
  glad
  compiler
  imgui
  gui
)
target_compile_definitions(main PRIVATE SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders") # expose to executables to load shaders at runtime

# strict warnings on project code only 
set(PROJECT_PEDANTIC_WARNINGS
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wpedantic>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Werror>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wshadow>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wconversion>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-float-conversion>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wsign-conversion>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wformat=2>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wundef>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wcast-qual>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-old-style-cast>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wnon-virtual-dtor>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wnull-dereference>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-double-promotion>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Woverloaded-virtual>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wimplicit-fallthrough>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-unused-parameter>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wmisleading-indentation>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wduplicated-cond>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wduplicated-branches>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wlogical-op>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wuseless-cast>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wzero-as-null-pointer-constant>
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra-semi>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<CXX_COMPILER_ID:MSVC>:/WX>
  $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)

target_compile_options(camera PRIVATE ${PROJECT_PEDANTIC_WARNINGS})
target_compile_options(compiler PRIVATE ${PROJECT_PEDANTIC_WARNINGS})
target_compile_options(gui PRIVATE ${PROJECT_PEDANTIC_WARNINGS})
target_compile_options(app PRIVATE ${PROJECT_PEDANTIC_WARNINGS}) 
target_compile_definitions(gui PRIVATE SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

target_compile_options(main PRIVATE ${PROJECT_PEDANTIC_WARNINGS})

# Testing Pipeline
enable_testing()

# test_camera - Camera class tests
add_executable(test_camera tests/test_camera.cpp)
target_link_libraries(test_camera PRIVATE camera)
target_include_directories(test_camera PRIVATE src)
add_test(NAME test_camera COMMAND test_camera)

# test_preprocessor - Preprocessor tests
add_executable(test_preprocessor tests/test_preprocessor.cpp)
target_link_libraries(test_preprocessor PRIVATE compiler)
target_include_directories(test_preprocessor PRIVATE src/compiler)
add_test(NAME test_preprocessor COMMAND test_preprocessor)

# test_compiler - Compiler tests (requires GL context)
add_executable(test_compiler tests/test_compiler.cpp)
target_link_libraries(test_compiler PRIVATE compiler glad glfw)
target_include_directories(test_compiler PRIVATE src)
add_test(NAME test_compiler COMMAND test_compiler)
set_tests_properties(test_compiler PROPERTIES SKIP_RETURN_CODE 77)

# test_gui - GUI tests (requires GL context)
add_executable(test_gui tests/test_gui.cpp)
target_link_libraries(test_gui PRIVATE gui camera compiler glad glfw imgui)
target_include_directories(test_gui PRIVATE src)
add_test(NAME test_gui COMMAND test_gui)
set_tests_properties(test_gui PROPERTIES SKIP_RETURN_CODE 77)

# test_gui_file_io - headless GUI file-loading tests
add_executable(test_gui_file_io tests/test_gui_file_io.cpp)
target_link_libraries(test_gui_file_io PRIVATE gui)
target_include_directories(test_gui_file_io PRIVATE src)
add_test(NAME test_gui_file_io COMMAND test_gui_file_io)

# test_mouse - Mouse interaction tests
add_executable(test_mouse tests/test_mouse.cpp)
target_link_libraries(test_mouse PRIVATE camera)
target_include_directories(test_mouse PRIVATE src)
add_test(NAME test_mouse COMMAND test_mouse)

# Custom target to run all tests
add_custom_target(run_tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS test_camera test_preprocessor test_compiler test_gui test_gui_file_io test_mouse
  COMMENT "Running all tests"
)
