#include "americasarmyshader.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/americas_army_shader.vshinc"
};
// clang format on

static constexpr char kTest[] = "Americasarmyshader";

Americasarmyshader::Americasarmyshader(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "AmericasArmyShader") {
  tests_[kTest] = [this]() { Test(); };
}

void Americasarmyshader::Initialize() {
  TestSuite::Initialize();

  results_.clear();
  computations_.clear();

  char buffer[128] = {0};
  {
    snprintf(buffer, sizeof(buffer), "Test with real inputs");
    auto prepare = [](const std::shared_ptr<VertexShaderProgram> &shader) {
      shader->SetUniformF(96, 0.1647059, 0.1647059, 0.1686275, 1.0);
      shader->SetUniformF(97, -728.0, -4058.0, 0.0, 0.0);

      shader->SetUniformF(121, 2.0, 2.0, 2.0, 1.0);
      shader->SetUniformF(135, 0.0, 0.5, 1.0, 3.0);
      shader->SetUniformF(136, 0.9983897, -0.0101479, -0.0558118, 0.0);
      shader->SetUniformF(137, -0.0567268, -0.178603, -0.9822846, 0.0);
      shader->SetUniformF(138, 0.0, 0.9838689, -0.1788911, 0.0);
      shader->SetUniformF(139, 31.5401363, -20.6333656, 67.6486282, 1.0);
      shader->SetUniformF(140, 0.0, 0.0, 0.0, 0.0);
      shader->SetUniformF(141, 0.0, 0.0, 0.0, 0.0);
      shader->SetUniformF(142, 0.0, 0.0, 0.0, 0.0);
      shader->SetUniformF(143, 0.0, 0.0, 0.0, 0.0);
      shader->SetUniformF(144, 0.0, 0.0, 0.0, 0.0);
      shader->SetUniformF(145, 1.0, 0.0, 0.0, 0.0);
      shader->SetUniformF(146, 0.0, 1.0, 0.0, 0.0);
      shader->SetUniformF(147, 0.0, 0.0, 1.0, 0.0);
    };
    results_.emplace_back(buffer);
    computations_.push_back({kShader, sizeof(kShader), prepare, nullptr, &results_.back()});
  }
}

void Americasarmyshader::Test() {
  host_.SetDiffuse(0x1AFE326C);
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
