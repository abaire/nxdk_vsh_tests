#pragma once
#include <memory>
#include <vector>

#include "nv2a_vsh_cpu.h"
#include "test_host.h"
#include "test_suite.h"

class CpuShaderTests : public TestSuite {
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
            const std::function<void(float*, const float*)>& cpu_op, uint32_t assert_line,
            const std::list<std::vector<float>>& additional_inputs = {});

 private:
  std::vector<float> test_values_;
};
