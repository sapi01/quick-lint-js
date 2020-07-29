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

#ifndef QUICK_LINT_JS_BUFFERING_VISITOR_H
#define QUICK_LINT_JS_BUFFERING_VISITOR_H

#include <quick-lint-js/language.h>
#include <quick-lint-js/lex.h>
#include <utility>
#include <vector>

namespace quick_lint_js {
class buffering_visitor {
 public:
  template <class Visitor>
  void move_into(Visitor &target) {
    for (auto &visit : this->visits_) {
      switch (visit.kind) {
        case visit_kind::end_of_module:
          target.visit_end_of_module();
          break;
        case visit_kind::enter_block_scope:
          target.visit_enter_block_scope();
          break;
        case visit_kind::enter_class_scope:
          target.visit_enter_class_scope();
          break;
        case visit_kind::enter_function_scope:
          target.visit_enter_function_scope();
          break;
        case visit_kind::enter_named_function_scope:
          target.visit_enter_named_function_scope(visit.name);
          break;
        case visit_kind::exit_block_scope:
          target.visit_exit_block_scope();
          break;
        case visit_kind::exit_class_scope:
          target.visit_exit_class_scope();
          break;
        case visit_kind::exit_function_scope:
          target.visit_exit_function_scope();
          break;
        case visit_kind::property_declaration:
          target.visit_property_declaration(visit.name);
          break;
        case visit_kind::variable_assignment:
          target.visit_variable_assignment(visit.name);
          break;
        case visit_kind::variable_use:
          target.visit_variable_use(visit.name);
          break;
        case visit_kind::variable_declaration:
          target.visit_variable_declaration(visit.name, visit.var_kind);
          break;
      }
    }
  }

  void visit_end_of_module() {
    this->visits_.emplace_back(visit_kind::end_of_module);
  }

  void visit_enter_block_scope() {
    this->visits_.emplace_back(visit_kind::enter_block_scope);
  }

  void visit_enter_class_scope() {
    this->visits_.emplace_back(visit_kind::enter_class_scope);
  }

  void visit_enter_function_scope() {
    this->visits_.emplace_back(visit_kind::enter_function_scope);
  }

  void visit_enter_named_function_scope(identifier name) {
    this->visits_.emplace_back(visit_kind::enter_named_function_scope, name);
  }

  void visit_exit_block_scope() {
    this->visits_.emplace_back(visit_kind::exit_block_scope);
  }

  void visit_exit_class_scope() {
    this->visits_.emplace_back(visit_kind::exit_class_scope);
  }

  void visit_exit_function_scope() {
    this->visits_.emplace_back(visit_kind::exit_function_scope);
  }

  void visit_property_declaration(identifier name) {
    this->visits_.emplace_back(visit_kind::property_declaration, name);
  }

  void visit_variable_assignment(identifier name) {
    this->visits_.emplace_back(visit_kind::variable_assignment, name);
  }

  void visit_variable_declaration(identifier name, variable_kind kind) {
    this->visits_.emplace_back(visit_kind::variable_declaration, name, kind);
  }

  void visit_variable_use(identifier name) {
    this->visits_.emplace_back(visit_kind::variable_use, name);
  }

 private:
  enum class visit_kind {
    end_of_module,
    enter_block_scope,
    enter_class_scope,
    enter_function_scope,
    enter_named_function_scope,
    exit_block_scope,
    exit_class_scope,
    exit_function_scope,
    property_declaration,
    variable_assignment,
    variable_use,
    variable_declaration,
  };

  struct visit {
    explicit visit(visit_kind kind) noexcept : kind(kind) {}

    explicit visit(visit_kind kind, identifier name) noexcept
        : kind(kind), name(name) {}

    explicit visit(visit_kind kind, identifier name,
                   variable_kind var_kind) noexcept
        : kind(kind), name(name), var_kind(var_kind) {}

    visit_kind kind;

    union {
      // enter_named_function_scope, property_declaration, variable_assignment,
      // variable_declaration, variable_use
      identifier name;
      static_assert(std::is_trivially_destructible_v<identifier>);
    };

    union {
      // variable_declaration
      variable_kind var_kind;
      static_assert(std::is_trivially_destructible_v<variable_kind>);
    };
  };

  std::vector<visit> visits_;
};
}  // namespace quick_lint_js

#endif