#include "cpu_shader_tests.h"

#include <pbkit/pbkit.h>

#include <cfloat>

#include "../test_host.h"
#include "SDL_stdinc.h"
#include "SDL_test_fuzzer.h"
#include "compareasint/compare_as_int.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"
#include "text_overlay.h"

static constexpr int kUnitsInLastPlace = 4;
static constexpr int kUnitsInLastPlaceLowPrecision = 0x200;

#define LOG_VERBOSE

//#define USE_EXCEPTIONAL_VALUES

//#define USE_FUZZ

static constexpr uint32_t kNumIterations = 32;
static constexpr uint32_t kIterationsPerFrame = 32;

static const uint32_t kExceptionalValues[] = {
    kPosInfInt, kNegInfInt, kPosNaNQInt,         kNegNaNQInt,         kPosMaxInt,          kNegMaxInt,
    kPosMinInt, kNegMinInt, kPosMaxSubnormalInt, kNegMaxSubnormalInt, kPosMinSubnormalInt, kNegMinSubnormalInt,
};

static const float kNormalValues[] = {
    0.0f,
    1.0f,
    0.123456789f,
    1.2345678e-20f,
    1.2345678e20f,
    3.2345678e-5f,
    3.2345678e5f,
    4.2345678e-4f,
    4.2345678e4f,
    5.8642111e-8f,
    586421118e8f,
    6.43210e-15f,
    6.2432105e15f,
    7.8901234e-3f,
    7.289012345e12f,
    8.90123456e-25f,
    8.90123456e25f,
    1.84467470083988e19f,
    -1.84467470083988e19f,
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
  // Asserts due to lack of log2f in nxdk.
  //  tests_["LOG"] = [this]() { TestLog(); };
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
#ifdef USE_EXCEPTIONAL_VALUES
  for (auto val : kExceptionalValues) {
    test_values_.emplace_back(*(float *)&val);
  }
#endif
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

  {
    char buf[256];
    snprintf(buf, sizeof(buf), "HW:  (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", hw_result[0], hw_result[1],
             hw_result[2], hw_result[3], *(uint32_t *)&hw_result[0], *(uint32_t *)&hw_result[1],
             *(uint32_t *)&hw_result[2], *(uint32_t *)&hw_result[3]);
    PrintMsg("%s", buf);
    snprintf(buf, sizeof(buf), "CPU: (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", cpu_result[0], cpu_result[1],
             cpu_result[2], cpu_result[3], *(uint32_t *)&cpu_result[0], *(uint32_t *)&cpu_result[1],
             *(uint32_t *)&cpu_result[2], *(uint32_t *)&cpu_result[3]);
    PrintMsg("%s", buf);
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
  snprintf(buf, sizeof(buf), "FAIL: %s\n  (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", name, a[0], a[1], a[2],
           a[3], *(uint32_t *)&a[0], *(uint32_t *)&a[1], *(uint32_t *)&a[2], *(uint32_t *)&a[3]);

  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);
  print_result_diff(hw_result, cpu_result);
}

static void print_assert_message(const char *name, const float *a, const float *b, const float *hw_result,
                                 const float *cpu_result) {
  char buf[256];
  snprintf(buf, sizeof(buf),
           "FAIL: %s\n  (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, 0x%08X)\n  (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, "
           "0x%08X)\n\n",
           name, a[0], a[1], a[2], a[3], *(uint32_t *)&a[0], *(uint32_t *)&a[1], *(uint32_t *)&a[2], *(uint32_t *)&a[3],
           b[0], b[1], b[2], b[3], *(uint32_t *)&b[0], *(uint32_t *)&b[1], *(uint32_t *)&b[2], *(uint32_t *)&b[3]);
  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);
  print_result_diff(hw_result, cpu_result);
}

static void print_assert_message(const char *name, const float *a, const float *b, const float *c,
                                 const float *hw_result, const float *cpu_result) {
  char buf[256];
  snprintf(buf, sizeof(buf),
           "FAIL: %s\n  (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, 0x%08X)\n  (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, "
           "0x%08X)\n",
           name, a[0], a[1], a[2], a[3], *(uint32_t *)&a[0], *(uint32_t *)&a[1], *(uint32_t *)&a[2], *(uint32_t *)&a[3],
           b[0], b[1], b[2], b[3], *(uint32_t *)&b[0], *(uint32_t *)&b[1], *(uint32_t *)&b[2], *(uint32_t *)&b[3]);
  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);

  snprintf(buf, sizeof(buf), "  (%g,%g,%g,%g\n   0x%08X, 0x%08X, 0x%08X, 0x%08X)\n\n", c[0], c[1], c[2], c[3],
           *(uint32_t *)&c[0], *(uint32_t *)&c[1], *(uint32_t *)&c[2], *(uint32_t *)&c[3]);
  PrintMsg("%s", buf);
  TextOverlay::Print("%s", buf);

  print_result_diff(hw_result, cpu_result);
}

static bool almost_equal(const float *cpu_result, const float *hw_result, int ulps = kUnitsInLastPlace) {
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

    // Zeros are allowed more absolute difference.
    uint32_t hw_int = *(uint32_t *)&hw_result[i];
    if (!hw_int || hw_int == 0x80000000) {
      if (fabsf(cpu_result[i]) > FLT_EPSILON) {
        return false;
      }
      continue;
    }

    if (!almost_equal_ulps(cpu_result[i], hw_result[i], ulps)) {
      return false;
    }
  }
  return true;
}

static bool TestBatch(TestHost &host, const char *name, uint32_t num_inputs, const uint32_t *shader,
                      uint32_t shader_size, const std::function<void(float *, const float *)> &cpu_op,
                      const std::list<std::vector<float>> &inputs, uint32_t *num_successes, uint32_t *num_tests,
                      bool low_precision) {
  std::list<TestHost::Results> results;

  {
    int j = 0;
    std::list<TestHost::Computation> computations;
    for (auto &input_set : inputs) {
      ASSERT(input_set.size() == num_inputs * 4);
      auto prepare = [j, input_set](const std::shared_ptr<VertexShaderProgram> &shader) {
#ifdef LOG_VERBOSE
        PrintMsg("HW INPUTS[%d]:\n", j);
#endif
        uint32_t offset = 0;
        for (auto input = 0; input < input_set.size() / 4; ++input, offset += 4) {
          shader->SetUniformF(96 + input, input_set[offset], input_set[offset + 1], input_set[offset + 2],
                              input_set[offset + 3]);

#ifdef LOG_VERBOSE
          PrintMsg("ARG%d  %g (0x%08X), %g (0x%08X), %g (0x%08X), %g (0x%08X)\n", input, input_set[offset + 0],
                   *(uint32_t *)&input_set[offset + 0], input_set[offset + 1], *(uint32_t *)&input_set[offset + 1],
                   input_set[offset + 2], *(uint32_t *)&input_set[offset + 2], input_set[offset + 3],
                   *(uint32_t *)&input_set[offset + 3]);
#endif
        }
      };

      results.emplace_back("result");
      computations.push_back({shader, shader_size, prepare, nullptr, &results.back()});
      ++j;
    }

    host.Compute(computations);
  }

  auto input_it = inputs.begin();
  int j = 0;
  for (auto &result : results) {
    ++*num_tests;
    const float *hw_result = result.cOut[0];
    const std::vector<float> &op_inputs = *input_it++;

#ifdef LOG_VERBOSE
    PrintMsg("CPU INPUTS[%d]:\n", j);
    for (auto idx = 0; idx < op_inputs.size() / 4; ++idx) {
      PrintMsg("CPU IN[%d]  %g (0x%08X), %g (0x%08X), %g (0x%08X), %g (0x%08X)\n", idx, op_inputs[idx * 4 + 0],
               *(uint32_t *)&op_inputs[idx * 4 + 0], op_inputs[idx * 4 + 1], *(uint32_t *)&op_inputs[idx * 4 + 1],
               op_inputs[idx * 4 + 2], *(uint32_t *)&op_inputs[idx * 4 + 2], op_inputs[idx * 4 + 3],
               *(uint32_t *)&op_inputs[idx * 4 + 3]);
    }
#endif

    VECTOR cpu_result;
    cpu_op(cpu_result, op_inputs.data());
    if (!almost_equal(cpu_result, hw_result, low_precision ? kUnitsInLastPlaceLowPrecision : kUnitsInLastPlace)) {
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
    } else {
#ifdef LOG_VERBOSE
      PrintMsg("Pass: %g (0x%08X), %g (0x%08X), %g (0x%08X), %g (0x%08X)\n", hw_result[0], *(uint32_t *)&hw_result[0],
               hw_result[1], *(uint32_t *)&hw_result[1], hw_result[2], *(uint32_t *)&hw_result[2], hw_result[3],
               *(uint32_t *)&hw_result[3]);
#endif
    }
    ++*num_successes;
    ++j;
  }

  pb_reset();
  host.Clear();
  TextOverlay::Print("%s: %d of %d\n", name, *num_successes, *num_tests);
  TextOverlay::Render();
  while (pb_finished()) {
  }

  return true;
}

void CpuShaderTests::Test(const char *name, uint32_t num_inputs, const uint32_t *shader, uint32_t shader_size,
                          const std::function<void(float *, const float *)> &cpu_op, uint32_t assert_line,
                          uint32_t flags, const std::list<std::vector<float>> &additional_inputs) {
  TextOverlay::Reset();

  // TODO: Test exceptional values
  uint32_t num_successes = 0;
  uint32_t num_tests = 0;

  if (!additional_inputs.empty()) {
    if (!TestBatch(host_, name, num_inputs, shader, shader_size, cpu_op, additional_inputs, &num_successes, &num_tests,
                   flags & CPUTF_LOW_PRECISION)) {
      TextOverlay::Render();
      while (pb_finished()) {
      }
      return;
    }
  }

  {
    const uint32_t num_values = test_values_.size();
    auto random_value = [this, flags, num_values]() {
      float ret = test_values_[SDLTest_RandomUint32() % num_values];
      if (flags & CPUTF_NO_NEGATIVES) {
        return fabs(ret);
      }
      return ret;
    };

    for (auto i = 0; i < kNumIterations; ++i) {
      std::list<std::vector<float>> inputs;
      for (auto j = 0; j < kIterationsPerFrame; ++j) {
        std::vector<float> input_set;
        for (auto input = 0; input < num_inputs; ++input) {
          input_set.emplace_back(random_value());
          if (flags & CPUTF_X_ONLY) {
            input_set.emplace_back(0.0f);
            input_set.emplace_back(0.0f);
            input_set.emplace_back(0.0f);
          } else {
            input_set.emplace_back(random_value());
            input_set.emplace_back(random_value());
            input_set.emplace_back(random_value());
          }
        }
        inputs.push_back(input_set);
      }

      if (!TestBatch(host_, name, num_inputs, shader, shader_size, cpu_op, inputs, &num_successes, &num_tests,
                     flags & CPUTF_LOW_PRECISION)) {
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

    if (!TestBatch(host_, name, num_inputs, shader, shader_size, cpu_op, inputs, &num_successes, &num_tests,
                   flags & CPUTF_LOW_PRECISION)) {
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
  TextOverlay::Print("%s: %d of %d Succeeded\n", name, num_successes, num_tests);
  TextOverlay::Render();
  while (pb_finished()) {
  }
}

void CpuShaderTests::TestExp() {
  std::list<std::vector<float>> explicit_tests = {
      {5.864211e-08f, 0.0f, 1.0f, 1.0f},
  };
  Test("EXP", 1, kExp, sizeof(kExp), nv2a_vsh_cpu_exp, __LINE__,
       CPUTF_NO_NEGATIVES | CPUTF_X_ONLY | CPUTF_LOW_PRECISION, explicit_tests);
}

void CpuShaderTests::TestLit() {
  std::list<std::vector<float>> explicit_tests = {
      {-1.844675e19f, 0.0f, 1.0f, 1.0f},
  };
  Test("LIT", 1, kLit, sizeof(kLit), nv2a_vsh_cpu_lit, __LINE__, CPUTF_NO_NEGATIVES | CPUTF_X_ONLY, explicit_tests);
}

void CpuShaderTests::TestLog() {
  //  // This halts on Xbox as log2f is not implemented and asserts.
  //  std::list<std::vector<float>> explicit_tests = {
  //      {-5.864211e16f, 0.00789012, 5.864211e-08f, -1.844675e19f},
  //  };
  //  Test("LOG", 1, kLog, sizeof(kLog), nv2a_vsh_cpu_log, __LINE__, CPUTF_X_ONLY | CPUTF_LOW_PRECISION,
  //  explicit_tests);
}

void CpuShaderTests::TestRcc() {
  std::list<std::vector<float>> explicit_tests = {
      {8.90123456e25f, 0.0f, 0.0f, 0.0f},
  };

  Test("RCC", 1, kRcc, sizeof(kRcc), nv2a_vsh_cpu_rcc, __LINE__, CPUTF_X_ONLY, explicit_tests);
}

void CpuShaderTests::TestRcp() {
  std::list<std::vector<float>> explicit_tests = {
      {-0.0f, -0.0f, -0.0f, -0.0f},
      {0.0f, 0.0f, 0.0f, 0.0f},
      {1.0f, 1.0f, 1.0f, 1.0f},
      {-1.0f, -1.0f, -1.0f, -1.0f},
  };
  Test("RCP", 1, kRcp, sizeof(kRcp), nv2a_vsh_cpu_rcp, __LINE__, CPUTF_X_ONLY, explicit_tests);
}

void CpuShaderTests::TestRsq() {
  std::list<std::vector<float>> explicit_tests = {
      {-0.0f, -0.0f, -0.0f, -0.0f}, {0.0f, 0.0f, 0.0f, 0.0f},           {1.0f, 1.0f, 1.0f, 1.0f},
      {-1.0f, -1.0f, -1.0f, -1.0f}, {8.90123456e25f, 0.0f, 0.0f, 0.0f}, {-8.90123456e25f, 0.0f, 0.0f, 0.0f},
  };

  Test("RSQ", 1, kRsq, sizeof(kRsq), nv2a_vsh_cpu_rsq, __LINE__, CPUTF_X_ONLY, explicit_tests);
}

void CpuShaderTests::TestAdd() { Test("ADD", 2, kAdd, sizeof(kAdd), nv2a_vsh_cpu_add, __LINE__); }

// void CpuShaderTests::TestArl() {}

void CpuShaderTests::TestDp3() {
  std::list<std::vector<float>> explicit_tests = {
      {8.901234e-25f, -1.844675e+19f, 1.844675e+19f, 5.864211e+16f, -1.844675e+19f, 42345.7f, 8.901235e+25f,
       -8.901234e-25f},
      {-8.901235e+25f, 6.432100e-15f, 5.864211e+16f, 1.844675e+19f, 1.844675e+19f, -6.432100e-15f, 1.234568e+20f,
       -0.123457f},
  };
  Test("DP3", 2, kDp3, sizeof(kDp3), nv2a_vsh_cpu_dp3, __LINE__, CPUTF_NONE, explicit_tests);
}

void CpuShaderTests::TestDp4() { Test("DP4", 2, kDp4, sizeof(kDp4), nv2a_vsh_cpu_dp4, __LINE__); }

void CpuShaderTests::TestDph() { Test("DPH", 2, kDph, sizeof(kDph), nv2a_vsh_cpu_dph, __LINE__); }

void CpuShaderTests::TestDst() { Test("DST", 2, kDst, sizeof(kDst), nv2a_vsh_cpu_dst, __LINE__); }

void CpuShaderTests::TestMad() {
  std::list<std::vector<float>> explicit_tests = {
      {-1.84467470083988e19f, -1.84467470083988e19f, 1.84467470083988e19f, 1.84467470083988e19f, 1.2345678e20f,
       -1.2345678e20f, 1.2345678e20f, -1.2345678e20f, 0.0f, 0.0f, 0.0f, 0.0f},
      {-1.84467470083988e19f, -1.84467470083988e19f, 1.84467470083988e19f, 1.84467470083988e19f, 1.2345678e20f,
       -1.2345678e20f, 1.2345678e20f, -1.2345678e20f, -0.0f, -0.0f, -0.0f, -0.0f},
      {-1.84467470083988e19f, -1.84467470083988e19f, 1.84467470083988e19f, 1.84467470083988e19f, 1.2345678e20f,
       -1.2345678e20f, 1.2345678e20f, -1.2345678e20f, 10.0f, 10.0f, 10.0f, 10.0f},
      {-1.84467470083988e19f, -1.84467470083988e19f, 1.84467470083988e19f, 1.84467470083988e19f, 1.2345678e20f,
       -1.2345678e20f, 1.2345678e20f, -1.2345678e20f, -10.0f, -10.0f, -10.0f, -10.0f},
  };
  Test("MAD", 3, kMad, sizeof(kMad), nv2a_vsh_cpu_mad, __LINE__, CPUTF_NONE, explicit_tests);
}

void CpuShaderTests::TestMax() { Test("MAX", 2, kMax, sizeof(kMax), nv2a_vsh_cpu_max, __LINE__); }

void CpuShaderTests::TestMin() { Test("MIN", 2, kMin, sizeof(kMin), nv2a_vsh_cpu_min, __LINE__); }

void CpuShaderTests::TestMov() { Test("MOV", 1, kMov, sizeof(kMov), nv2a_vsh_cpu_mov, __LINE__); }

void CpuShaderTests::TestMul() {
  std::list<std::vector<float>> explicit_tests = {
      {-1.84467470083988e19f, -1.84467470083988e19f, 1.84467470083988e19f, 1.84467470083988e19f, 1.2345678e20f,
       -1.2345678e20f, 1.2345678e20f, -1.2345678e20f},
  };
  Test("MUL", 2, kMul, sizeof(kMul), nv2a_vsh_cpu_mul, __LINE__, CPUTF_NONE, explicit_tests);
}

void CpuShaderTests::TestSge() {
  std::list<std::vector<float>> explicit_tests = {
      {-0.0f, 0.0f, -0.0f, 0.0f, 0.0f, 0.0f, -0.0f, -0.0f},
  };
  Test("SGE", 2, kSge, sizeof(kSge), nv2a_vsh_cpu_sge, __LINE__, CPUTF_NONE, explicit_tests);
}

void CpuShaderTests::TestSlt() {
  std::list<std::vector<float>> explicit_tests = {
      {-0.0f, 0.0f, -0.0f, 0.0f, 0.0f, 0.0f, -0.0f, -0.0f},
      {-0.0f, 0.0f, -0.0f, 0.0f, 0.1f, 0.1f, -0.1f, -0.1f},
      {-0.0f, 0.0f, -0.0f, 0.0f, 5.8642111e-8f, 5.8642111e-8f, -5.8642111e-8f, -5.8642111e-8f},
  };
  Test("SLT", 2, kSlt, sizeof(kSlt), nv2a_vsh_cpu_slt, __LINE__, CPUTF_NONE, explicit_tests);
}
