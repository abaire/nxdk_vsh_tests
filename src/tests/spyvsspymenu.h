#pragma once
#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class Spyvsspymenu : public TestSuite {
 public:
  Spyvsspymenu(TestHost &host, std::string output_dir);
  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test();

 private:
  void *attribute_buffer_{nullptr};
};
