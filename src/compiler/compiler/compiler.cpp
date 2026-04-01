#include "compiler.h"
#include "preprocessor/preprocessor.h"

#include <filesystem>
#include <glad/glad.h>
#include <iostream>

Compiler::CompileOutput
Compiler::compile(const std::filesystem::path &source_file, GLenum shader_type,
                  bool verbose) {

  if (verbose)
    std::cout << "started compilation of: " << source_file.string()
              << std::endl;

  Compiler::CompileOutput output{false, 0u, ""};

  // preprocess source
  Preprocessor::Output pp = _preprocessor.process_source(source_file, verbose);
  if (!pp.success) {
    output.error = pp.error;
    return output;
  }

  if (verbose)
    std::cout << "Preprocessed source: \n" << pp.processed_source << std::endl;

  if (glCreateShader == nullptr) {
    output.error = "OpenGL shader API is unavailable: GL context not initialized";
    return output;
  }

  // Create output shader
  const std::string label = source_file.string();
  GLuint shader = glCreateShader(shader_type);
  if (shader == 0u) {
    output.error = "glCreateShader returned 0 for: " + label;
    return output;
  }

  // get source + compile
  const char *src = pp.processed_source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  // Check compilation result
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

  if (ok != GL_TRUE) {
    // Get log length then fetch full log
    GLint log_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

    std::string log;
    if (log_len > 1) {
      log.resize(static_cast<size_t>(log_len));
      GLsizei written = 0;
      glGetShaderInfoLog(shader, log_len, &written, log.data());
      if (written > 0 && static_cast<size_t>(written) < log.size()) {
        log.resize(static_cast<size_t>(written));
      }
    }

    glDeleteShader(shader);

    output.error = "ERROR::SHADER::" + label + "::COMPILATION_FAILED\n" + log;
    return output;
  }

  output.success = true;
  output.shader = shader;
  output.error.clear();
  return output;
}
