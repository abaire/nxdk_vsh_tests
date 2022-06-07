#include "mac_add_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

// clang format off
static constexpr uint32_t kShader[] = {
    #include "shaders/mac_add_v3_v4__od0.vshinc"
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

  char buffer[32] = {0};

  {
    uint32_t a = 0x1AFE326C;
    uint32_t b = 0x21004A33;
    snprintf(buffer, sizeof(buffer), "0x%08X,0x%08X xyzw xy", a, b);
    auto prepare = [this, a, b](const std::shared_ptr<VertexShaderProgram> &_shader) {
      host_.SetDiffuse(a);
      host_.SetSpecular(b);
    };
    results_.push_back({buffer, 0, 0});
    computations_.push_back({kShader, sizeof(kShader), prepare, &results_.back()});
  }
}


void MacAddTests::Test() {
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
