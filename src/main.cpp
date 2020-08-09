// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew Glazar
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <quick-lint-js/error.h>
#include <quick-lint-js/language.h>
#include <quick-lint-js/lex.h>
#include <quick-lint-js/lint.h>
#include <quick-lint-js/location.h>
#include <quick-lint-js/options.h>
#include <quick-lint-js/parse.h>
#include <quick-lint-js/text-error-reporter.h>
#include <string>

namespace quick_lint_js {
namespace {
void process_file(const char *path, bool print_parser_visits);
std::string read_file(const char *path);
}  // namespace
}  // namespace quick_lint_js

int main(int argc, char **argv) {
  quick_lint_js::options o = quick_lint_js::parse_options(argc, argv);
  if (!o.error_unrecognized_options.empty()) {
    for (const auto &option : o.error_unrecognized_options) {
      std::cerr << "error: unrecognized option: " << option << '\n';
    }
    return 1;
  }
  if (o.files_to_lint.empty()) {
    std::cerr << "error: expected file name\n";
    return 1;
  }

  for (const char *file_to_lint : o.files_to_lint) {
    quick_lint_js::process_file(file_to_lint, o.print_parser_visits);
  }

  return 0;
}

namespace quick_lint_js {
namespace {
class debug_visitor {
 public:
  void visit_end_of_module() { std::cerr << "end of module\n"; }

  void visit_enter_block_scope() { std::cerr << "entered block scope\n"; }

  void visit_enter_class_scope() { std::cerr << "entered class scope\n"; }

  void visit_enter_for_scope() { std::cerr << "entered for scope\n"; }

  void visit_enter_function_scope() { std::cerr << "entered function scope\n"; }

  void visit_enter_named_function_scope(identifier) {
    std::cerr << "entered named function scope\n";
  }

  void visit_exit_block_scope() { std::cerr << "exited block scope\n"; }

  void visit_exit_class_scope() { std::cerr << "exited class scope\n"; }

  void visit_exit_for_scope() { std::cerr << "exited for scope\n"; }

  void visit_exit_function_scope() { std::cerr << "exited function scope\n"; }

  void visit_property_declaration(identifier name) {
    std::cerr << "property declaration: " << name.string_view() << '\n';
  }

  void visit_variable_assignment(identifier name) {
    std::cerr << "variable assignment: " << name.string_view() << '\n';
  }

  void visit_variable_declaration(identifier name, variable_kind) {
    std::cerr << "variable declaration: " << name.string_view() << '\n';
  }

  void visit_variable_use(identifier name) {
    std::cerr << "variable use: " << name.string_view() << '\n';
  }
};

template <class Visitor1, class Visitor2>
class multi_visitor {
 public:
  explicit multi_visitor(Visitor1 *visitor_1, Visitor2 *visitor_2) noexcept
      : visitor_1_(visitor_1), visitor_2_(visitor_2) {}

  void visit_end_of_module() {
    this->visitor_1_->visit_end_of_module();
    this->visitor_2_->visit_end_of_module();
  }

  void visit_enter_block_scope() {
    this->visitor_1_->visit_enter_block_scope();
    this->visitor_2_->visit_enter_block_scope();
  }

  void visit_enter_class_scope() {
    this->visitor_1_->visit_enter_class_scope();
    this->visitor_2_->visit_enter_class_scope();
  }

  void visit_enter_for_scope() {
    this->visitor_1_->visit_enter_for_scope();
    this->visitor_2_->visit_enter_for_scope();
  }

  void visit_enter_function_scope() {
    this->visitor_1_->visit_enter_function_scope();
    this->visitor_2_->visit_enter_function_scope();
  }

  void visit_enter_named_function_scope(identifier name) {
    this->visitor_1_->visit_enter_named_function_scope(name);
    this->visitor_2_->visit_enter_named_function_scope(name);
  }

  void visit_exit_block_scope() {
    this->visitor_1_->visit_exit_block_scope();
    this->visitor_2_->visit_exit_block_scope();
  }

  void visit_exit_class_scope() {
    this->visitor_1_->visit_exit_class_scope();
    this->visitor_2_->visit_exit_class_scope();
  }

  void visit_exit_for_scope() {
    this->visitor_1_->visit_exit_for_scope();
    this->visitor_2_->visit_exit_for_scope();
  }

  void visit_exit_function_scope() {
    this->visitor_1_->visit_exit_function_scope();
    this->visitor_2_->visit_exit_function_scope();
  }

  void visit_property_declaration(identifier name) {
    this->visitor_1_->visit_property_declaration(name);
    this->visitor_2_->visit_property_declaration(name);
  }

  void visit_variable_assignment(identifier name) {
    this->visitor_1_->visit_variable_assignment(name);
    this->visitor_2_->visit_variable_assignment(name);
  }

  void visit_variable_declaration(identifier name, variable_kind kind) {
    this->visitor_1_->visit_variable_declaration(name, kind);
    this->visitor_2_->visit_variable_declaration(name, kind);
  }

  void visit_variable_use(identifier name) {
    this->visitor_1_->visit_variable_use(name);
    this->visitor_2_->visit_variable_use(name);
  }

 private:
  Visitor1 *visitor_1_;
  Visitor2 *visitor_2_;
};

void process_file(const char *path, bool print_parser_visits) {
  std::string source = read_file(path);
  text_error_reporter error_reporter(std::cerr, source.c_str(), path);
  parser p(source.c_str(), &error_reporter);
  linter l(&error_reporter);
  if (print_parser_visits) {
    debug_visitor logger;
    multi_visitor visitor(&logger, &l);
    p.parse_and_visit_module(visitor);
  } else {
    p.parse_and_visit_module(l);
  }
}

std::string read_file(const char *path) {
  FILE *file = std::fopen(path, "rb");
  if (!file) {
    std::cerr << "error: failed to open " << path << ": "
              << std::strerror(errno) << '\n';
    exit(1);
  }

  if (std::fseek(file, 0, SEEK_END) == -1) {
    std::cerr << "error: failed to seek to end of " << path << ": "
              << std::strerror(errno) << '\n';
    exit(1);
  }

  long file_size = std::ftell(file);
  if (file_size == -1) {
    std::cerr << "error: failed to get size of " << path << ": "
              << std::strerror(errno) << '\n';
    exit(1);
  }

  if (std::fseek(file, 0, SEEK_SET) == -1) {
    std::cerr << "error: failed to seek to beginning of " << path << ": "
              << std::strerror(errno) << '\n';
    exit(1);
  }

  std::string contents;
  contents.resize(file_size);
  std::size_t read_size = std::fread(contents.data(), 1, file_size, file);
  contents.resize(read_size);

  if (std::fclose(file) == -1) {
    std::cerr << "error: failed to close " << path << ": "
              << std::strerror(errno) << '\n';
    exit(1);
  }

  return contents;
}
}  // namespace
}  // namespace quick_lint_js
