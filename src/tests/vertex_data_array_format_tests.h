#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class VertexDataArrayFormatTests : public TestSuite {
 public:
  struct TestCase {
    const char *name;
    uint32_t format;
    uint32_t count;
    uint32_t stride;
    std::function<void(void *, const VECTOR)> generator;
  };

 public:
  VertexDataArrayFormatTests(TestHost &host, std::string output_dir);
  void Initialize() override;
  void Deinitialize() override;

 private:
  //  void Test(const TestCase &test_case);
  void TestF();
  void TestS1();
  void TestS32K();

 private:
  float *vertex_buffer_{nullptr};
  void *attribute_buffer_{nullptr};
};
