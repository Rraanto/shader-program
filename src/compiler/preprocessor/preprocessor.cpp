#include "preprocessor.h"
#include <filesystem>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>

bool Preprocessor::_is_file_in_stack(const std::filesystem::path &file) {
  // loop over stack and query equivalence of paths
  for (const std::filesystem::path &match : this->_expansion_stack) {
    if (std::filesystem::equivalent(match, file))
      return true;
  }
  return false;
}

static inline bool _is_line_include(const std::string &line) {
  // checks if a line corresponds to an include directive
  // Skip leading whitespace
  size_t pos = line.find_first_not_of(" \t");
  if (pos == std::string::npos)
    return false;

  // Check if line starts with "#include"
  const std::string include_prefix = "#include";
  if (line.compare(pos, include_prefix.size(), include_prefix) != 0)
    return false;

  // Look for opening quote after #include
  size_t quote_pos = line.find('"', pos + include_prefix.size());
  return quote_pos != std::string::npos;
}

static inline std::string _extract_include_string(const std::string &line) {
  // extracts the include string from an include directive:
  // (#include "some_file.glsl") -> some_file.glsl
  //
  // in practice used when line is already known to be include directive
  // Find first quote
  size_t first_quote = line.find('"');
  if (first_quote == std::string::npos)
    throw std::runtime_error("Not a valid include directive: " + line);

  // Find second quote
  size_t second_quote = line.find('"', first_quote + 1);
  if (second_quote == std::string::npos)
    throw std::runtime_error("Not a valid include directive: " + line);

  // Extract filename between quotes
  return line.substr(first_quote + 1, second_quote - first_quote - 1);
}

Preprocessor::Output
Preprocessor::_expand(const std::filesystem::path &file_path, bool verbose) {
  // Recursive expansion of included files
  // using the preprocessor stack

  std::string error_msg = "";
  std::string source = "";

  // store and check against canonical paths
  std::filesystem::path file_path_canonical =
      std::filesystem::weakly_canonical(file_path);
  if (!std::filesystem::exists(file_path_canonical)) {
    error_msg = "File does not exist: " + file_path.string();
    return Preprocessor::Output{false, "", error_msg};
  }

  // check recursive include from stack
  if (this->_is_file_in_stack(file_path_canonical)) {
    error_msg = "Recursive include: " + file_path_canonical.string();
    return Preprocessor::Output{false, "", error_msg};
  }

  this->_expansion_stack.push_back(file_path_canonical);
  struct StackPopGuard {
    std::vector<std::filesystem::path> &stack;
    ~StackPopGuard() {
      if (!stack.empty()) {
        stack.pop_back();
      }
    }
  } stack_pop_guard{this->_expansion_stack};

  // read file (known to exist) line by line
  std::ifstream file(file_path);
  std::string line;
  while (std::getline(file, line)) {

    // recursive expand if include directive
    if (_is_line_include(line)) {

      if (verbose)
        std::cout << "Found include directive: " << line << std::endl;

      std::string include_string = _extract_include_string(line);

      // optionally found include path
      std::optional<std::filesystem::path> include_path =
          this->_resolve_include(file_path_canonical, include_string);

      if (!include_path) {
        error_msg = "Include not found: " + include_string;
        return Preprocessor::Output{false, "", error_msg};
      }

      if (verbose)
        std::cout << "Resolved include: " << "\"" << include_string << "\""
                  << std::endl;

      Preprocessor::Output out_recursive = this->_expand(*include_path);

      // if included file contains error
      if (!out_recursive.success) {
        error_msg = out_recursive.error;
        return Preprocessor::Output{false, "", error_msg};
      }

      source += out_recursive.processed_source;
    }

    // add line as is otherwise
    else {
      source += line + "\n";
    }
  }
  return Preprocessor::Output{true, source, error_msg};
}

std::optional<std::filesystem::path>
Preprocessor::_resolve_include(const std::filesystem::path &include_from,
                               const std::string &include_string) {

  std::filesystem::path included_file;
  std::filesystem::path file_name(include_string);

  // search in current directory first:
  included_file = include_from.parent_path() / file_name;

  if (std::filesystem::is_regular_file(included_file))
    return included_file;

  // visit all include directories if not found
  for (const std::filesystem::path &include_dir : this->_include_dirs) {
    included_file = include_dir / file_name;
    if (std::filesystem::is_regular_file(included_file))
      return included_file;
  }

  // none found
  return std::nullopt;
}

// public
Preprocessor::Output
Preprocessor::process_source(const std::filesystem::path &file_path,
                             bool verbose) {
  // Each compile should start with a clean include-expansion stack.
  this->_expansion_stack.clear();
  return this->_expand(file_path, verbose);
}
