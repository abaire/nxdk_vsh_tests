#ifndef NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H
#define NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class VertexBuffer;

// Tests behavior when vertex attributes are not provided but are used by shaders.
class AttributeCarryoverTests : public TestSuite {
 public:
  AttributeCarryoverTests(TestHost &host, std::string output_dir);
  void Initialize() override;

 private:
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H
