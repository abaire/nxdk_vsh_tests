#include "ilu_rcp_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

// Infinities
static const uint32_t kPosInf = 0x7F800000;
static const uint32_t kNegInf = 0xFF800000;

// Quiet NaN on x86
static const uint32_t kPosNaNQ = 0x7FC00000;
static const uint32_t kNegNaNQ = 0xFFC00000;

// Max normal
static const uint32_t kPosMax = 0x7F7FFFFF;
static const uint32_t kNegMax = 0xFF7FFFFF;

// Min normal
static const uint32_t kPosMin = 0x00800000;
static const uint32_t kNegMin = 0x80800000;

// Max subnormal
static const uint32_t kPosMaxSubnormal = 0x007FFFFF;
static const uint32_t kNegMaxSubnormal = 0x807FFFFF;

// Min subnormal
static const uint32_t kPosMinSubnormal = 0x00000001;
static const uint32_t kNegMinSubnormal = 0x80000001;

// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/ilu_rcp.vshinc"
};
// clang format on

static constexpr char kTest[] = "IluRcpTests";

IluRcpTests::IluRcpTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "ILU RCP Tests") {
  tests_[kTest] = [this]() { Test(); };
}

void IluRcpTests::Initialize() {
  TestSuite::Initialize();

  results_.clear();
  computations_.clear();

  char buffer[128] = {0};

  {
    XboxMath::vector_t a = {1.0f, 2.123f, 0.0f, 0.0f};
    a[2] = *(float *)&kPosMin;
    a[3] = *(float *)&kPosMax;

    XboxMath::vector_t b = {-1.0f, -2.123f, 0.0f, 0.0f};
    b[2] = *(float *)&kNegMin;
    b[3] = *(float *)&kNegMax;

    XboxMath::vector_t c = {0.0f, 0.0f, 0.0f, 0.0f};
    c[1] = *(float *)&kPosInf;
    c[2] = *(float *)&kNegInf;
    c[3] = *(float *)&kPosNaNQ;

    XboxMath::vector_t d = {0.0f, 0.0f, 0.0f, 0.0f};
    d[0] = *(float *)&kNegNaNQ;
    d[1] = *(float *)&kNegMaxSubnormal;
    d[2] = *(float *)&kPosMinSubnormal;
    d[3] = *(float *)&kNegMinSubnormal;

    snprintf(buffer, sizeof(buffer), "%f,%f,Min,Max\n%f,%f,-Min,-Max\n0,Inf,-Inf,Nan\n-Nan,-MaxSub,MinSub,-MinSub",
             a[0], a[1], b[0], b[1]);
    auto prepare = [a, b, c, d](const std::shared_ptr<VertexShaderProgram> &shader) {
      shader->SetUniform4F(96, a);
      shader->SetUniform4F(97, b);
      shader->SetUniform4F(98, c);
      shader->SetUniform4F(99, d);
    };
    results_.emplace_back(buffer, RES_ALL);
    computations_.push_back({kShader, sizeof(kShader), prepare, nullptr, &results_.back()});
  }
}

void IluRcpTests::Test() {
  host_.SetDiffuse(0x1AFE326C);
  host_.Compute(computations_);
  host_.DrawResults(results_, allow_saving_, output_dir_, kTest);
}
