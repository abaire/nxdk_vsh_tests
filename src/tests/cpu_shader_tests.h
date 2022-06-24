#pragma once
#include <memory>
#include <vector>

#include "nv2a_vsh_cpu.h"
#include "test_host.h"
#include "test_suite.h"

class CpuShaderTests : public TestSuite {
 public:
  enum TestFlags {
    CPUTF_NONE = 0,
    CPUTF_NO_NEGATIVES = 1 << 0,
    CPUTF_X_ONLY = 1 << 1,
    CPUTF_LOW_PRECISION = 1 << 2,
  };

 public:
  CpuShaderTests(TestHost& host, std::string output_dir);
  void Initialize() override;

 private:
  void TestExp();
  void TestLit();
  void TestLog();
  void TestRcc();
  void TestRcp();
  void TestRsq();
  void TestAdd();
  //  void TestArl();
  void TestDp3();
  void TestDp4();
  void TestDph();
  void TestDst();
  void TestMad();
  void TestMax();
  void TestMin();
  void TestMov();
  void TestMul();
  void TestSge();
  void TestSlt();

  void Test(const char* name, uint32_t num_inputs, const uint32_t* shader, uint32_t shader_size,
            const std::function<void(float*, const float*)>& cpu_op, uint32_t assert_line, uint32_t flags = CPUTF_NONE,
            const std::list<std::vector<float>>& additional_inputs = {});

 private:
  std::vector<float> test_values_;
};
