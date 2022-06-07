#include "mac_mov_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

//// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/mac_mov.vshinc"
};
//// clang format on

static constexpr char kTest[] = "mov";

MACMovTests::MACMovTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "MAC mov") {
  tests_[kTest] = [this]() { Test(); };
}

void MACMovTests::Initialize() {
  TestSuite::Initialize();

  results_.clear();
  computations_.clear();

  char buffer[128] = {0};

  {
    VECTOR a = {1.0f, 2.0f, -3.0f, -4.12345f};
    snprintf(buffer, sizeof(buffer), "%f,%f,%f,%f", a[0], a[1], a[2], a[3]);
    auto prepare = [a](const std::shared_ptr<VertexShaderProgram> &shader) {
      shader->SetUniform4F(96, a);
    };
    results_.emplace_back(buffer);
    computations_.push_back({kShader, sizeof(kShader), prepare, &results_.back()});
  }
}


void MACMovTests::Test() {
  host_.SetDiffuse(0x1AFE326C);
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
