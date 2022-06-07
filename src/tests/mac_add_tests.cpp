#include "mac_add_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

// clang format off
static constexpr uint32_t kShader[] = {
    #include "shaders/mac_add.vshinc"
};
// clang format on

static constexpr char kTest[] = "MacAddTests";

MacAddTests::MacAddTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "MAC Add Tests") {
  tests_[kTest] = [this]() { Test(); };
}

void MacAddTests::Initialize() {
  TestSuite::Initialize();

  results_.clear();
  computations_.clear();

  char buffer[128] = {0};

  {
    VECTOR a = {1.0f, 2.0f, -1.0f, -2.33f};
    VECTOR b = {1000.5f, 2424.99f, 1.0f, -100.0f};
    snprintf(buffer, sizeof(buffer), "%f,%f,%f,%f +\n%f,%f,%f,%f", a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]);
    auto prepare = [a, b](const std::shared_ptr<VertexShaderProgram> &shader) {
      shader->SetUniform4F(96, a);
      shader->SetUniform4F(97, b);
    };
    results_.emplace_back(buffer);
    computations_.push_back({kShader, sizeof(kShader), prepare, &results_.back()});
  }
}


void MacAddTests::Test() {
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
