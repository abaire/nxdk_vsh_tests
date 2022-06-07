#include "mac_mov_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

//// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/mac_mov_v3__od0_od1xy.vshinc"
};
//// clang format on

static constexpr char kTest[] = "mov";

MACMovTests::MACMovTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "MAC mov") {
  tests_[kTest] = [this]() { Test(); };
}

void MACMovTests::Initialize() {
  TestSuite::Initialize();

  char buffer[32] = {0};

  snprintf(buffer, sizeof(buffer), "0x%08X xyzw xy", 0x1AFE326C);
  results_.push_back({buffer, 0, 0});
  computations_.push_back({kShader, sizeof(kShader), TestHost::NullPrepare, &results_.back()});
}


void MACMovTests::Test() {
  host_.SetDiffuse(0x1AFE326C);
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
