#pragma once
#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class PairedIluTests : public TestSuite {
 public:
  PairedIluTests(TestHost &host, std::string output_dir);
  void Initialize() override;

 private:
  void Test();

 private:
  std::list<TestHost::Computation> computations_;
  std::list<TestHost::Results> results_;
};
