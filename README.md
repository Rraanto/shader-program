# Shader editor and visualiser

This project is an OpenGL shader editor, compiler and visualiser: Under the hood, it renders a fullscreen quad, drives a fragment shader with camera and time uniforms, and lets the user recompile the fragment shader live from an ImGui editor.

![Screenshot](./screenshots/screenshot.png)

# Installation

The project has been consistently managed with `Cmake` which is used to build. It is recommended to make a specific `/build/` folder, from which: 

```bash
cmake ..
```
should generate a `Makefile`, and allow to run: 

```bash
make
```

Which will generate the `main` executable that launches the app, and other test executables.

Although I have tried to make pendantic compilation rules with `C++ 17`, with hopes for best portability between compilers/platforms, but have not had the opportunity to test.
