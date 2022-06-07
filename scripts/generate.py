#!/usr/bin/env python3

"""Utility to generate new tests."""

import argparse
import os
import re
import sys
from typing import List, Tuple

_CONTINUATION_RE = re.compile(r"^(\s*)(.*?)\s*\\\s*$")

_TESTS_DIR = "src/tests"
_SHADERS_DIR = "src/shaders"

_SOURCE_TEMPLATE = """#include "_HEADER_"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

_SHADERS_
static constexpr char kTest[] = "_TEST_NAME_";

_CLASSNAME_::_CLASSNAME_(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "_SUITE_NAME_") {
  tests_[kTest] = [this]() { Test(); };
}

void _CLASSNAME_::Initialize() {
  TestSuite::Initialize();

  results_.clear();
  computations_.clear();

  char buffer[128] = {0};

  {
    VECTOR a = {1.0f, 2.0f, 3.0f, 4.0f};
    snprintf(buffer, sizeof(buffer), "%f,%f,%f,%f", a[0], a[1], a[2], a[3]);
    auto prepare = [a](const std::shared_ptr<VertexShaderProgram> &shader) {
      shader->SetUniform4F(96, a);
    };
    results_.emplace_back(buffer);
    computations_.push_back({kShader, sizeof(kShader), prepare, &results_.back()});
  }
}


void _CLASSNAME_::Test() {
  host_.SetDiffuse(0x1AFE326C);
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
"""

_HEADER_TEMPLATE = """#pragma once
#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class _CLASSNAME_ : public TestSuite {
 public:
  _CLASSNAME_(TestHost &host, std::string output_dir);
  void Initialize() override;

 private:
  void Test();

 private:
  std::list<TestHost::Computation> computations_;
  std::list<TestHost::Results> results_;
};
"""


_SHADER_TEMPLATE = """; Values from c[188], c[189], c[190], c[191] will be captured
#output matrix4 188
"""

def _make_filename(name: str, extension: str) -> str:
    name = name.lower().replace(" ", "_")
    return f"{name}.{extension}"


def _generate_shader(filename: str, force: bool = False):
    full_path = os.path.join(_SHADERS_DIR, filename)
    if not force and os.path.exists(full_path):
        raise Exception(f"Shader already exists at {full_path}")

    with open(full_path, "w", encoding="ascii") as outfile:
        print(f"; TODO: IMPLEMENT ME", file=outfile)


def _make_shader_variable_name(shader_filename: str) -> str:
    ret = shader_filename[:-4]
    ret = ret.replace("__", "= ").replace("_", " ").title()
    ret = ret.replace("= ", "_").replace(" ", "")
    return f"k{ret}"


def _generate_test(
    name: str, shaders: List[str], force: bool = False
) -> Tuple[List[str], List[str], str]:
    source = _make_filename(name, "cpp")
    cpp_path = os.path.join(_TESTS_DIR, source)
    if not force and os.path.exists(cpp_path):
        raise Exception(f"Test already exists at {cpp_path}")

    header = _make_filename(name, "h")
    h_path = os.path.join(_TESTS_DIR, header)
    if not force and os.path.exists(cpp_path):
        raise Exception(f"Test already exists at {h_path}")

    classname = name.title().replace(" ", "")
    test_name = classname
    suite_name = name

    shader_string = ""
    if shaders:
        components = ["// clang format off"]

        if len(shaders) == 1:
            components.append("static constexpr uint32_t kShader[] = {")
            components.append(f'    #include "shaders/{shaders[0]}inc"')
            components.append("};")
        else:
            for shader in shaders:
                shader_name = _make_shader_variable_name(shader)
                components.append(f"static constexpr uint32_t {shader_name}[] = " + "{")
                components.append(f'    #include "shaders/{shader}inc"')
                components.append("};")

        components.append("// clang format on")
        shader_string = "\n".join(components)
        shader_string += "\n"

    with open(cpp_path, "w", encoding="ascii") as outfile:
        content = _SOURCE_TEMPLATE.replace("_SUITE_NAME_", suite_name)
        content = content.replace("_TEST_NAME_", test_name)
        content = content.replace("_CLASSNAME_", classname)
        content = content.replace("_HEADER_", header)
        content = content.replace("_SHADERS_", shader_string)
        outfile.write(content)

    with open(h_path, "w", encoding="ascii") as outfile:
        content = _HEADER_TEMPLATE.replace("_CLASSNAME_", classname)
        outfile.write(content)

    return [source], [header], classname


def _update_makefile(test_sources: List[str], shader_sources: List[str]):
    with open("Makefile", "r", encoding="ascii") as infile:
        content = infile.readlines()

    new_source_files = set(
        [f"$(SRCDIR)/tests/{name}" for name in test_sources if name.endswith(".cpp")]
    )
    new_shader_files = set([f"$(SRCDIR)/shaders/{name}inc" for name in shader_sources])

    new_content = []

    i = 0
    while i < len(content):
        line = content[i]
        i += 1
        new_content.append(line)

        if line.startswith("SRCS = \\"):
            while i < len(content):
                match = _CONTINUATION_RE.match(content[i])
                if not match:
                    break
                existing = match.group(2)
                new_source_files.discard(existing)
                new_content.append(content[i])
                i += 1
            assert i < len(content)
            previous_end = content[i].rstrip()
            new_source_files.discard(previous_end.lstrip())
            if new_source_files:
                new_content.append(previous_end + " \\\n")
                new_content.append(
                    " \\\n".join(sorted([f"\t{file}" for file in new_source_files]))
                )
                new_content.append("\n")
            else:
                new_content.append(content[i])
            i += 1

        if line.startswith("NV2A_VSH_OBJS = \\"):
            while i < len(content):
                match = _CONTINUATION_RE.match(content[i])
                if not match:
                    break
                new_shader_files.discard(match.group(1))
                new_content.append(content[i])
                i += 1
            assert i < len(content)
            previous_end = content[i].rstrip()
            new_shader_files.discard(previous_end.lstrip())
            if new_source_files:
                new_content.append(previous_end + " \\\n")
                new_content.append(
                    " \\\n".join(sorted([f"\t{file}" for file in new_shader_files]))
                )
                new_content.append("\n")
            else:
                new_content.append(content[i])
            i += 1

    content = "".join(new_content)
    with open("Makefile", "w", encoding="ascii") as outfile:
        outfile.write(content)


def _update_main(classname: str, test_sources: List[str]):
    with open("src/main.cpp", encoding="ascii") as infile:
        content = infile.readlines()

    new_header_files = [
        f'#include "tests/{name}"' for name in test_sources if name.endswith(".h")
    ]
    assert len(new_header_files) == 1

    new_content = []

    # It is assumed that the include files are all at the top of the file.
    last_include_index = -1
    i = 0
    while i < len(content):
        line = content[i]
        i += 1
        new_content.append(line)

        if line.startswith("#include "):
            if line.startswith(new_header_files[0]):
                return
            last_include_index = i

        if line.startswith("static void register_suites("):
            # Skip the declaration.
            line = content[i + 1]
            if "{" not in line:
                continue

            while i < len(content) and not content[i].startswith("}"):
                new_content.append(content[i])
                i += 1
            i += 1
            new_content.append("  {\n")
            new_content.append(
                f"    auto suite = std::make_shared<{classname}>(host, output_directory);\n"
            )
            new_content.append("    test_suites.push_back(suite);\n")
            new_content.append("  }\n")
            new_content.append("}\n")

    assert last_include_index >= 0
    new_content.insert(last_include_index, new_header_files[0] + "\n")
    content = "".join(new_content)
    with open(os.path.join("src", "main.cpp"), "w", encoding="ascii") as outfile:
        outfile.write(content)


def _main(args):
    test_dir = os.path.abspath(_TESTS_DIR)
    if not os.path.isdir(test_dir):
        print(
            f"'{test_dir}' is not a directory, this script must be run from the root of the repository."
        )
        return 1

    shader_dir = os.path.abspath(_SHADERS_DIR)
    if not os.path.isdir(shader_dir):
        print(
            f"'{shader_dir}' is not a directory, this script must be run from the root of the repository."
        )
        return 1

    shader_sources = []
    for shader in args.shaders:
        filename = _make_filename(shader, "vsh")
        shader_sources.append(filename)
        _generate_shader(filename, args.force)

    test_sources, test_headers, classname = _generate_test(
        args.name, shader_sources, args.force
    )

    _update_main(classname, test_headers)
    _update_makefile(test_sources, shader_sources)

    return 0


if __name__ == "__main__":

    def _parse_args():
        parser = argparse.ArgumentParser()

        parser.add_argument(
            "name",
            metavar="test_name",
            help="The name of the test.",
        )

        parser.add_argument(
            "shaders",
            nargs="*",
            metavar="shader_filename",
            help="Any number of stub shader files to generate.",
        )

        parser.add_argument(
            "-f",
            "--force",
            action="store_true",
            help="Overwrite any existing files.",
        )

        return parser.parse_args()

    sys.exit(_main(_parse_args()))
