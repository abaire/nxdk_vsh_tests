#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

//! Tests how exceptional floating point values (e.g., NaN) are interpreted by
//! the vertex shader.
class ExceptionalFloatTests : public TestSuite {
 public:
  ExceptionalFloatTests(TestHost &host, std::string output_dir);

 private:
  void Test();
};
