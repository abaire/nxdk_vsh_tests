#include "exceptional_float_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "pushbuffer.h"
#include "shaders/vertex_shader_program.h"

// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/exceptional_float_passthrough.vshinc"
};
// clang format on

static constexpr char kTestName[] = "ExceptionalFloat";

static float f(uint32_t v) { return *((float *)&v); }

ExceptionalFloatTests::ExceptionalFloatTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Exceptional Float") {
  tests_[kTestName] = [this]() { Test(); };
}

void ExceptionalFloatTests::Test() {
  // Infinities
  static constexpr uint32_t posInf = 0x7F800000;
  static constexpr uint32_t negInf = 0xFF800000;
  // Quiet NaN on x86
  static constexpr uint32_t posNanQ = 0x7FC00000;
  static constexpr uint32_t negNanQ = 0xFFC00000;
  // Max normal
  static constexpr uint32_t posMax = 0x7F7FFFFF;
  static constexpr uint32_t negMax = 0xFF7FFFFF;
  // Min normal
  static constexpr uint32_t posMinNorm = 0x00800000;
  static constexpr uint32_t negMinNorm = 0x80800000;
  // Max subnormal
  static constexpr uint32_t posMaxSubNorm = 0x007FFFFF;
  static constexpr uint32_t negMaxSubNorm = 0x807FFFFF;
  // Min subnormal
  static constexpr uint32_t posMinSubNorm = 0x00000001;
  static constexpr uint32_t negMinSubNorm = 0x80000001;

  std::list<TestHost::Computation> computations;
  std::list<TestHost::Results> results;

  struct ExceptionalTest {
    const char* name;
    uint32_t values[4];
  };

  static constexpr ExceptionalTest kExceptionalTests[] = {
      {"Inf, -Inf, NaN, -Nan", {posInf, negInf, posNanQ, negNanQ}},
      {"Max, -Max, Min, -Min", {posMax, negMax, posMinNorm, negMinNorm}},
      {"MaxSub, -MaxSub, MinSub, -MinSub", {posMaxSubNorm, negMaxSubNorm, posMinSubNorm, negMinSubNorm}},
  };

  for (const auto& test : kExceptionalTests) {
    auto draw = [this, &test]() {
      host_.SetNormal(f(test.values[0]), f(test.values[1]), f(test.values[2]));
      host_.SetDiffuse(f(test.values[0]), f(test.values[1]), f(test.values[2]), f(test.values[3]));
      host_.SetSpecular(f(test.values[0]), f(test.values[1]), f(test.values[2]), f(test.values[3]));
      host_.SetDiffuse(f(test.values[0]), f(test.values[1]), f(test.values[2]), f(test.values[3]));
      host_.SetSpecular(f(test.values[0]), f(test.values[1]), f(test.values[2]), f(test.values[3]));
      host_.SetFogCoord(f(test.values[0]));
      host_.SetVertex(f(test.values[0]), f(test.values[1]), f(test.values[2]), f(test.values[3]));
    };
    results.emplace_back(test.name);
    computations.push_back({kShader, sizeof(kShader), nullptr, draw, &results.back()});
  }

  host_.Compute(computations);
  host_.DrawResults(results, allow_saving_, output_dir_, "Float");
}
