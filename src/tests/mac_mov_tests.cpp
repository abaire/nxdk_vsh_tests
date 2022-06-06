#include "mac_mov_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

//// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/mov_r0.vshinc"
};
//// clang format on

static constexpr char kTest[] = "mov";

MACMovTests::MACMovTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "MAC mov") {
  tests_[kTest] = [this]() { Test(); };
}

void MACMovTests::Initialize() {
  TestSuite::Initialize();

  auto shader = host_.PrepareCalculation(kShader, sizeof(kShader));
}


void MACMovTests::Test() {
  VECTOR diffuse;
  VECTOR specular;
  VECTOR r0;
  VECTOR r1;

  host_.SetDiffuse(0xFFFE326C);
  host_.Calculate(diffuse, specular, r0, r1, allow_saving_, output_dir_, kTest);
}
