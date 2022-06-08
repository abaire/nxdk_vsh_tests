#include "paired_ilu_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/paired_ilu_non_r1_temp_out.vshinc"
};
// clang format on

static constexpr char kTest[] = "PairedIluTests";

PairedIluTests::PairedIluTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Paired ILU Tests") {
  tests_[kTest] = [this]() { Test(); };
}

void PairedIluTests::Initialize() {
  TestSuite::Initialize();

  results_.clear();
  computations_.clear();

  char buffer[128] = {0};

  {
    VECTOR a = {25.0f, 1.123f, 2.123f, 3.123f};
    snprintf(buffer, sizeof(buffer), "%f,%f,%f,%f", a[0], a[1], a[2], a[3]);
    auto prepare = [a](const std::shared_ptr<VertexShaderProgram> &shader) { shader->SetUniform4F(96, a); };
    auto result_labels = std::map<uint32_t, std::string>{
        {0, "R1.x = RSQ\n   "},
        {1, "R1 = Input\n   "},
        {2, "R1 (R10.x = RSQ)\n   "},
        {3, "R10 (R10.x = RSQ)\n   "},
    };
    results_.emplace_back(buffer, RES_ALL, result_labels);
    computations_.push_back({kShader, sizeof(kShader), prepare, &results_.back()});
  }
}

void PairedIluTests::Test() {
  host_.SetDiffuse(0x1AFE326C);
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
