# quick-lint-js finds bugs in JavaScript programs.
# Copyright (C) 2020  Matthew Glazar
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

function (quick_lint_js_check_designated_initializers OUT)
  check_cxx_source_compiles(
    "struct s { int m; }; int main() { s x{.m = 0}; return x.m; }"
    "${OUT}"
  )
endfunction ()

function (quick_lint_js_enable_char8_t_if_supported)
  check_cxx_compiler_flag(-fchar8_t QUICK_LINT_JS_HAVE_FCHAR8_T)
  if (QUICK_LINT_JS_HAVE_FCHAR8_T)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fchar8_t>)
  endif ()
endfunction ()

function (quick_lint_js_configure_exception_handling)
  if (MSVC)
    add_compile_options(/EHcs)
  endif ()
endfunction ()

function (quick_lint_js_set_cxx_standard)
  set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
  if (cxx_std_20 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_STANDARD 20)
  else ()
    set(CMAKE_CXX_STANDARD 17)
    quick_lint_js_check_designated_initializers(
      QUICK_LINT_JS_COMPILER_SUPPORTS_DESIGNATED_INITIALIZERS
    )
    if (NOT "${QUICK_LINT_JS_COMPILER_SUPPORTS_DESIGNATED_INITIALIZERS}")
      message(
        FATAL_ERROR
        "C++ compiler does not support designated initializers (either GNU extension or C++20)"
      )
    endif ()
  endif ()

  set(CMAKE_CXX_STANDARD "${CMAKE_CXX_STANDARD}" PARENT_SCOPE)
  set(CMAKE_CXX_STANDARD_REQUIRED "${CMAKE_CXX_STANDARD_REQUIRED}" PARENT_SCOPE)
endfunction ()

function (quick_lint_js_add_warning_options_if_supported)
  cmake_parse_arguments("" "" "" "INTERFACE;PRIVATE;PUBLIC" ${ARGN})
  set(TARGETS "${_UNPARSED_ARGUMENTS}")

  foreach (STYLE INTERFACE PRIVATE PUBLIC)
    quick_lint_js_get_supported_warning_options(
      "${_${STYLE}}"
      WARNING_OPTIONS_TO_ADD
    )
    if (WARNING_OPTIONS_TO_ADD)
      target_compile_options("${TARGETS}" "${STYLE}" "${WARNING_OPTIONS_TO_ADD}")
    endif ()
  endforeach ()
endfunction ()

function (quick_lint_js_get_supported_warning_options WARNING_OPTIONS OUT_SUPPORTED_WARNING_OPTIONS)
  set(SUPPORTED_WARNING_OPTIONS)
  foreach (WARNING_OPTION IN LISTS WARNING_OPTIONS)
    quick_lint_js_warning_option_var_name("${WARNING_OPTION}" WARNING_OPTION_VAR_NAME)
    check_cxx_compiler_flag("${WARNING_OPTION}" "${WARNING_OPTION_VAR_NAME}")
    if ("${${WARNING_OPTION_VAR_NAME}}")
      list(APPEND SUPPORTED_WARNING_OPTIONS "${WARNING_OPTION}")
    endif ()
  endforeach ()
  set("${OUT_SUPPORTED_WARNING_OPTIONS}" "${SUPPORTED_WARNING_OPTIONS}" PARENT_SCOPE)
endfunction ()

function (quick_lint_js_warning_option_var_name WARNING_OPTION OUT_VAR_NAME)
  set(VAR_NAME "${WARNING_OPTION}")
  string(TOUPPER "${VAR_NAME}" VAR_NAME)
  string(REGEX REPLACE "[-/]" _ VAR_NAME "${VAR_NAME}")
  set(VAR_NAME "QUICK_LINT_JS_HAVE_WARNING_OPTION_${VAR_NAME}")
  set("${OUT_VAR_NAME}" "${VAR_NAME}" PARENT_SCOPE)
endfunction ()

function (quick_lint_js_work_around_implicit_link_directories)
  # HACK(strager): Work around weird CMake behaviors when mixing multiple
  # languages (e.g. C and C++).
  #
  # Let's say the user asks CMake to compile C code with compiler X and C++ code
  # with compiler Y. Let's say a CMake file says we need to link an executable
  # which mixes C code and C++ code. CMake does the following:
  #
  # 1. Ask compiler X for its implicit linker flags.
  # 2. Use compiler Y to link the executable. Use some of compiler X's implicit
  #    linker flags.
  #
  # This is a problem when using compiler X's flags break compiler Y in some
  # way. In the wild, we observed that, if compiler X is GCC 7 and compiler Y is
  # GCC 8, CMake links the executable with GCC 8 but uses GCC 7's path to
  # libstdc++. This means that sources are compiled with GCC 8's libstdc++ but
  # link with GCC 7's libstdc++.
  #
  # Work around this by having compiler Y's flags take priority over compiler
  # X's flags. We need to clear CMAKE_<LANG>_IMPLICIT_LINK_DIRECTORIES;
  # otherwise, CMake ignores the link_directories command because it thinks the
  # compiler will already include those directories.
  #
  # See this issue for more details:
  # https://github.com/quick-lint/quick-lint-js/issues/9
  link_directories(${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
  set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "" PARENT_SCOPE)
endfunction ()
