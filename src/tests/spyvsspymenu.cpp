#include "spyvsspymenu.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/spyvsspymenu.vshinc"
};
// clang format on

static constexpr char kTest[] = "Menu";

Spyvsspymenu::Spyvsspymenu(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "SpyVsSpy") {
  tests_[kTest] = [this]() { Test(); };
}

static void GenerateShort(void *out, short a, short b, short c, short d) {
  auto buf = reinterpret_cast<short *>(out);
  for (uint32_t i = 0; i < 4; ++i) {
    *buf++ = a;
    *buf++ = b;
    *buf++ = c;
    *buf++ = d;
  }
}

void Spyvsspymenu::Initialize() {
  TestSuite::Initialize();
  attribute_buffer_ = MmAllocateContiguousMemoryEx(1024, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
}

void Spyvsspymenu::Deinitialize() {
  TestSuite::Deinitialize();
  if (attribute_buffer_) {
    MmFreeContiguousMemory(attribute_buffer_);
    attribute_buffer_ = nullptr;
  }
}

void Spyvsspymenu::Test() {
  std::list<TestHost::Computation> computations;
  std::list<TestHost::Results> results;

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + 4,
               MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_S32K) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, 4) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, 8));
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + 4, VRAM_ADDR(attribute_buffer_));
  pb_end(p);

  char buffer[128] = {0};
  {
    snprintf(buffer, sizeof(buffer), "89, 5156, -151, 417");
    auto prepare = [this](const std::shared_ptr<VertexShaderProgram> &shader) {
      GenerateShort(attribute_buffer_, 89, 5156, -151, 417);
      shader->SetUniformF(58, 320.00, -240.00, 16777215.00, 0.00);
      shader->SetUniformF(59, 320.03125, 240.03125, 0.00, 0.00);
      shader->SetUniformF(96, 1.00, 0.00, 0.00, 0.00);
      shader->SetUniformF(97, 0.00, 1.00, 0.00, 0.00);
      shader->SetUniformF(98, 0.00, 0.00, 1.00, 0.00);
      shader->SetUniformF(99, -16.8876857758, -18.2681617737, 3.6487216949, 1.00);
      shader->SetUniformF(120, 1.00, 0.0000610352, 0.0009765625, 1.00);
      shader->SetUniformF(125, 1.00, 1.00, 1.00, 0.00);
    };

    auto result_labels = std::map<uint32_t, std::string>{
        {0, "oPos\n   "},
        {1, "R1.x\n   "},
        {2, "R11\n   "},
        {3, "R0\n   "},
    };
    results.emplace_back(buffer, RES_ALL, result_labels);
    computations.push_back({kShader, sizeof(kShader), prepare, &results.back()});
  }

  host_.ComputeWithVertexBuffer(computations);
  host_.DrawResults(results, allow_saving_, output_dir_, kTest);
}
