#pragma once
#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class MacAddTests : public TestSuite {
 public:
  MacAddTests(TestHost &host, std::string output_dir);
  void Initialize() override;

 private:
  void Test();

 private:
  std::list<TestHost::Computation> computations_;
  std::list<TestHost::Results> results_;
};
