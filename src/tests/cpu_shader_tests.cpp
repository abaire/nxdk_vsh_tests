#include "cpu_shader_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "SDL_stdinc.h"
#include "SDL_test_fuzzer.h"
#include "compareasint/compare_as_int.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"
#include "text_overlay.h"

static constexpr int kUnitsInLastPlace = 4;

//#define LOG_VERBOSE
//#define USE_FUZZ

static constexpr uint32_t kNumIterations = 32;
static constexpr uint32_t kIterationsPerFrame = 32;

static const uint32_t kExceptionalValues[] = {
    kPosInfInt, kNegInfInt, kPosNaNQInt,         kNegNaNQInt,         kPosMaxInt,          kNegMaxInt,
    kPosMinInt, kNegMinInt, kPosMaxSubnormalInt, kNegMaxSubnormalInt, kPosMinSubnormalInt, kNegMinSubnormalInt,
};

static const float kNormalValues[] = {
    0.0f,          1.0f,          0.123456789f,    1.2345678e-20f,  1.2345678e20f,  3.2345678e-5f,
    3.2345678e5f,  4.2345678e-4f, 4.2345678e4f,    5.8642111e-8f,   586421118e8f,   6.43210e-15f,
    6.2432105e15f, 7.8901234e-3f, 7.289012345e12f, 8.90123456e-25f, 8.90123456e25f,
};

// clang format off
static constexpr uint32_t kExp[] = {
#include "shaders/ilu_exp_passthrough.vshinc"
};
static constexpr uint32_t kLit[] = {
#include "shaders/ilu_lit_passthrough.vshinc"
};
static constexpr uint32_t kLog[] = {
#include "shaders/ilu_log_passthrough.vshinc"
};
static constexpr uint32_t kRcc[] = {
#include "shaders/ilu_rcc_passthrough.vshinc"
};
static constexpr uint32_t kRcp[] = {
#include "shaders/ilu_rcp_passthrough.vshinc"
};
static constexpr uint32_t kRsq[] = {
#include "shaders/ilu_rsq_passthrough.vshinc"
};
static constexpr uint32_t kAdd[] = {
#include "shaders/mac_add_passthrough.vshinc"
};
static constexpr uint32_t kArl[] = {
#include "shaders/mac_arl_passthrough.vshinc"
};
static constexpr uint32_t kDp3[] = {
#include "shaders/mac_dp3_passthrough.vshinc"
};
static constexpr uint32_t kDp4[] = {
#include "shaders/mac_dp4_passthrough.vshinc"
};
static constexpr uint32_t kDph[] = {
#include "shaders/mac_dph_passthrough.vshinc"
};
static constexpr uint32_t kDst[] = {
#include "shaders/mac_dst_passthrough.vshinc"
};
static constexpr uint32_t kMad[] = {
#include "shaders/mac_mad_passthrough.vshinc"
};
static constexpr uint32_t kMax[] = {
#include "shaders/mac_max_passthrough.vshinc"
};
static constexpr uint32_t kMin[] = {
#include "shaders/mac_min_passthrough.vshinc"
};
static constexpr uint32_t kMov[] = {
#include "shaders/mac_mov_passthrough.vshinc"
};
static constexpr uint32_t kMul[] = {
#include "shaders/mac_mul_passthrough.vshinc"
};
static constexpr uint32_t kSge[] = {
#include "shaders/mac_sge_passthrough.vshinc"
};
static constexpr uint32_t kSlt[] = {
#include "shaders/mac_slt_passthrough.vshinc"
};
// clang format on

CpuShaderTests::CpuShaderTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "CPU Shader Tests") {
  tests_["EXP"] = [this]() { TestExp(); };
  tests_["LIT"] = [this]() { TestLit(); };
  tests_["LOG"] = [this]() { TestLog(); };
  tests_["RCC"] = [this]() { TestRcc(); };
  tests_["RCP"] = [this]() { TestRcp(); };
  tests_["RSQ"] = [this]() { TestRsq(); };
  tests_["ADD"] = [this]() { TestAdd(); };
  //  tests_["ARL"] = [this]() { TestArl(); };
  tests_["DP3"] = [this]() { TestDp3(); };
  tests_["DP4"] = [this]() { TestDp4(); };
  tests_["DPH"] = [this]() { TestDph(); };
  tests_["DST"] = [this]() { TestDst(); };
  tests_["MAD"] = [this]() { TestMad(); };
  tests_["MAX"] = [this]() { TestMax(); };
  tests_["MIN"] = [this]() { TestMin(); };
  tests_["MOV"] = [this]() { TestMov(); };
  tests_["MUL"] = [this]() { TestMul(); };
  tests_["SGE"] = [this]() { TestSge(); };
  tests_["SLT"] = [this]() { TestSlt(); };
}

void CpuShaderTests::Initialize() {
  TestSuite::Initialize();

  test_values_.clear();
  for (auto val : kExceptionalValues) {
    test_values_.emplace_back(*(float *)&val);
  }
  for (auto val : kNormalValues) {
    test_values_.emplace_back(val);
    test_values_.emplace_back(-1.0f * val);
  }
}

static void print_result_diff(const float *hw_result, const float *cpu_result) {
  uint32_t as_int_hw[4];
  uint32_t as_int_cpu[4];
  for (uint32_t i = 0; i < 4; ++i) {
    as_int_hw[i] = *(uint32_t *)&hw_result[i];
    as_int_cpu[i] = *(uint32_t *)&cpu_result[i];
  }

  TextOverlay::Print("   HW                   CPU\n");
  TextOverlay::Print("X: %- 18.10g %- 18.10g\n", hw_result[0], cpu_result[0]);
  TextOverlay::Print("   0x%08X           0x%08X\n", as_int_hw[0], as_int_cpu[0]);
  TextOverlay::Print("Y: %- 18.10g %- 18.10g\n", hw_result[1], cpu_result[1]);
  TextOverlay::Print("   0x%08X           0x%08X\n", as_int_hw[1], as_int_cpu[1]);
  TextOverlay::Print("Z: %- 18.10g %- 18.10g\n", hw_result[2], cpu_result[2]);
  TextOverlay::Print("   0x%08X           0x%08X\n", as_int_hw[2], as_int_cpu[2]);
  TextOverlay::Print("W: %- 18.10g %- 18.10g\n", hw_result[3], cpu_result[3]);
  TextOverlay::Print("   0x%08X           0x%08X\n", as_int_hw[3], as_int_cpu[3]);
}

static void print_assert_message(const char *name, const float *a, const float *hw_result, const float *cpu_result) {
  char buf[256];
  snprintf(buf, sizeof(buf), "FAIL: %s\n  (%g,%g,%g,%g\n   %08X,%08X,%08X,%08X)\n", name, a[0], a[1], a[2], a[3],
           *(uint32_t *)&a[0], *(uint32_t *)&a[1], *(uint32_t *)&a[2], *(uint32_t *)&a[3]);

  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);
  print_result_diff(hw_result, cpu_result);
}

static void print_assert_message(const char *name, const float *a, const float *b, const float *hw_result,
                                 const float *cpu_result) {
  char buf[256];
  snprintf(buf, sizeof(buf),
           "FAIL: %s\n  (%g,%g,%g,%g\n   %08X,%08X,%08X,%08X)\n  (%g,%g,%g,%g\n   %08X,%08X,%08X,%08X)\n\n", name, a[0],
           a[1], a[2], a[3], *(uint32_t *)&a[0], *(uint32_t *)&a[1], *(uint32_t *)&a[2], *(uint32_t *)&a[3], b[0], b[1],
           b[2], b[3], *(uint32_t *)&b[0], *(uint32_t *)&b[1], *(uint32_t *)&b[2], *(uint32_t *)&b[3]);
  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);
  print_result_diff(hw_result, cpu_result);
}

static void print_assert_message(const char *name, const float *a, const float *b, const float *c,
                                 const float *hw_result, const float *cpu_result) {
  char buf[256];
  snprintf(buf, sizeof(buf),
           "FAIL: %s\n  (%g,%g,%g,%g\n   %08X,%08X,%08X,%08X)\n  (%g,%g,%g,%g\n   %08X,%08X,%08X,%08X)\n", name, a[0],
           a[1], a[2], a[3], *(uint32_t *)&a[0], *(uint32_t *)&a[1], *(uint32_t *)&a[2], *(uint32_t *)&a[3], b[0], b[1],
           b[2], b[3], *(uint32_t *)&b[0], *(uint32_t *)&b[1], *(uint32_t *)&b[2], *(uint32_t *)&b[3]);
  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);

  snprintf(buf, sizeof(buf), "  (%g,%g,%g,%g\n   %08X,%08X,%08X,%08X)\n\n", c[0], c[1], c[2], c[3], *(uint32_t *)&c[0],
           *(uint32_t *)&c[1], *(uint32_t *)&c[2], *(uint32_t *)&c[3]);
  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);

  print_result_diff(hw_result, cpu_result);
}

static bool almost_equal(const float *cpu_result, const float *hw_result) {
  for (uint32_t i = 0; i < 4; ++i) {
    bool nan_a = isnan(cpu_result[i]);
    bool nan_b = isnan(hw_result[i]);
    if (nan_a != nan_b) {
      return false;
    }
    // Consider any NaN representations to be equivalent.
    if (nan_a) {
      return true;
    }

    if (!almost_equal_ulps(cpu_result[i], hw_result[i], kUnitsInLastPlace)) {
      return false;
    }
  }
  return true;
}

static bool TestBatch(TestHost &host, const char *name, uint32_t num_inputs, const uint32_t *shader,
                      uint32_t shader_size, const std::function<void(float *, const float *)> &cpu_op,
                      const std::list<std::vector<float>> &inputs, uint32_t *num_successes, uint32_t *num_tests) {
  std::list<TestHost::Computation> computations;
  std::list<TestHost::Results> results;

  int j = 0;
  for (auto &input_set : inputs) {
    auto prepare = [&inputs, j, num_inputs](const std::shared_ptr<VertexShaderProgram> &shader) {
      auto input_it = inputs.begin();
      std::advance(input_it, j);
      const std::vector<float> &op_inputs = *input_it;

#ifdef LOG_VERBOSE
      PrintMsg("HW INPUTS[%d]:\n", j);
      for (auto foo = 0; foo < op_inputs.size() / 4; ++foo) {
        PrintMsg("  %g, %g, %g, %g\n", op_inputs[foo * 4 + 0], op_inputs[foo * 4 + 1], op_inputs[foo * 4 + 2],
                 op_inputs[foo * 4 + 3]);
      }
#endif

      uint32_t offset = 0;
      for (auto input = 0; input < num_inputs; ++input, offset += 4) {
        shader->SetUniformF(96 + input, op_inputs[offset], op_inputs[offset + 1], op_inputs[offset + 2],
                            op_inputs[offset + 3]);
      }
    };

    results.emplace_back("result");
    computations.push_back({shader, shader_size, prepare, &results.back()});
  }

  host.Compute(computations);

  auto input_it = inputs.begin();
  j = 0;
  for (auto &result : results) {
    ++*num_tests;
    const float *hw_result = result.c188;

    const std::vector<float> &op_inputs = *input_it;

#ifdef LOG_VERBOSE
    PrintMsg("CPU INPUTS[%d]:\n", j);
    for (auto foo = 0; foo < op_inputs.size() / 4; ++foo) {
      PrintMsg("  %g, %g, %g, %g\n", op_inputs[foo * 4 + 0], op_inputs[foo * 4 + 1], op_inputs[foo * 4 + 2],
               op_inputs[foo * 4 + 3]);
    }
#endif

    VECTOR cpu_result;
    cpu_op(cpu_result, op_inputs.data());
    if (!almost_equal(cpu_result, hw_result)) {
      pb_reset();
      host.Clear();
      TextOverlay::Reset();

      switch (num_inputs) {
        case 1:
          print_assert_message(name, &op_inputs[0], hw_result, cpu_result);
          break;

        case 2:
          print_assert_message(name, &op_inputs[0], &op_inputs[4], hw_result, cpu_result);
          break;

        case 3:
          print_assert_message(name, &op_inputs[0], &op_inputs[4], &op_inputs[8], hw_result, cpu_result);
          break;

        default:
          ASSERT(!"Invalid number of inputs.");
      }

      TextOverlay::Print("at %d/%d\n", *num_successes, *num_tests);
      return false;
    }
    ++*num_successes;
  }

  pb_reset();
  host.Clear();
  TextOverlay::Print("%d of %d\n", *num_successes, *num_tests);
  TextOverlay::Render();
  while (pb_finished()) {
  }

  return true;
}

void CpuShaderTests::Test(const char *name, uint32_t num_inputs, const uint32_t *shader, uint32_t shader_size,
                          const std::function<void(float *, const float *)> &cpu_op, uint32_t assert_line) {
  TextOverlay::Reset();

  // TODO: Test exceptional values
  uint32_t num_successes = 0;
  uint32_t num_tests = 0;

  {
    const uint32_t num_values = test_values_.size();
    for (auto i = 0; i < kNumIterations; ++i) {
      std::list<std::vector<float>> inputs;
      for (auto j = 0; j < kIterationsPerFrame; ++j) {
        std::vector<float> input_set;
        for (auto input = 0; input < num_inputs; ++input) {
          input_set.emplace_back(test_values_[SDLTest_RandomUint32() % num_values]);
          input_set.emplace_back(test_values_[SDLTest_RandomUint32() % num_values]);
          input_set.emplace_back(test_values_[SDLTest_RandomUint32() % num_values]);
          input_set.emplace_back(test_values_[SDLTest_RandomUint32() % num_values]);
          inputs.push_back(input_set);
        }
      }

      if (!TestBatch(host_, name, num_inputs, shader, shader_size, cpu_op, inputs, &num_successes, &num_tests)) {
        TextOverlay::Render();
        while (pb_finished()) {
        }
        return;
      }
    }
  }

#ifdef USE_FUZZ
  for (auto i = 0; i < kNumIterations; ++i) {
    std::list<std::vector<float>> inputs;
    for (auto j = 0; j < kIterationsPerFrame; ++j) {
      std::vector<float> input_set;
      for (auto input = 0; input < num_inputs; ++input) {
        input_set.emplace_back(RandomFloat());
        input_set.emplace_back(RandomFloat());
        input_set.emplace_back(RandomFloat());
        input_set.emplace_back(RandomFloat());
      }
      inputs.push_back(input_set);
    }

    if (!TestBatch(host_, name, num_inputs, shader, shader_size, cpu_op, inputs, &num_successes, &num_tests)) {
      pb_draw_text_screen();
      while (pb_finished()) {
      }
      return;
    }
  }
#endif

  pb_wait_for_vbl();
  pb_reset();
  host_.Clear();
  TextOverlay::Reset();
  TextOverlay::Print("%d of %d Succeeded\n", num_successes, num_tests);
  TextOverlay::Render();
  while (pb_finished()) {
  }
}

void CpuShaderTests::TestExp() { Test("EXP", 1, kExp, sizeof(kExp), nv2a_vsh_cpu_exp, __LINE__); }

void CpuShaderTests::TestLit() { Test("LIT", 1, kLit, sizeof(kLit), nv2a_vsh_cpu_lit, __LINE__); }

void CpuShaderTests::TestLog() { Test("LOG", 1, kLog, sizeof(kLog), nv2a_vsh_cpu_log, __LINE__); }

void CpuShaderTests::TestRcc() { Test("RCC", 1, kRcc, sizeof(kRcc), nv2a_vsh_cpu_rcc, __LINE__); }

void CpuShaderTests::TestRcp() { Test("RCP", 1, kRcp, sizeof(kRcp), nv2a_vsh_cpu_rcp, __LINE__); }

void CpuShaderTests::TestRsq() { Test("RSQ", 1, kRsq, sizeof(kRsq), nv2a_vsh_cpu_rsq, __LINE__); }

void CpuShaderTests::TestAdd() { Test("ADD", 2, kAdd, sizeof(kAdd), nv2a_vsh_cpu_add, __LINE__); }

// void CpuShaderTests::TestArl() {}

void CpuShaderTests::TestDp3() { Test("DP3", 2, kDp3, sizeof(kDp3), nv2a_vsh_cpu_dp3, __LINE__); }

void CpuShaderTests::TestDp4() { Test("DP4", 2, kDp4, sizeof(kDp4), nv2a_vsh_cpu_dp4, __LINE__); }

void CpuShaderTests::TestDph() { Test("DPH", 2, kDph, sizeof(kDph), nv2a_vsh_cpu_dph, __LINE__); }

void CpuShaderTests::TestDst() { Test("DST", 2, kDst, sizeof(kDst), nv2a_vsh_cpu_dst, __LINE__); }

void CpuShaderTests::TestMad() { Test("MAD", 3, kMad, sizeof(kMad), nv2a_vsh_cpu_mad, __LINE__); }

void CpuShaderTests::TestMax() { Test("MAX", 2, kMax, sizeof(kMax), nv2a_vsh_cpu_max, __LINE__); }

void CpuShaderTests::TestMin() { Test("MIN", 2, kMin, sizeof(kMin), nv2a_vsh_cpu_min, __LINE__); }

void CpuShaderTests::TestMov() { Test("MOV", 1, kMov, sizeof(kMov), nv2a_vsh_cpu_mov, __LINE__); }

void CpuShaderTests::TestMul() { Test("MUL", 2, kMul, sizeof(kMul), nv2a_vsh_cpu_mul, __LINE__); }

void CpuShaderTests::TestSge() { Test("SGE", 2, kSge, sizeof(kSge), nv2a_vsh_cpu_sge, __LINE__); }

void CpuShaderTests::TestSlt() { Test("SLT", 2, kSlt, sizeof(kSlt), nv2a_vsh_cpu_slt, __LINE__); }
