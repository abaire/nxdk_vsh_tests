#include "vertex_data_array_format_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"

// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/vertex_data_array_format_passthrough.vshinc"
};
// clang format on

static void GenerateFloat(void* out, float a, float b, float c, float d);
static void GenerateShort(void* out, short a, short b, short c, short d);
static void GenerateDWORD(void* out, uint32_t a, uint32_t b, uint32_t c, uint32_t d);

VertexDataArrayFormatTests::VertexDataArrayFormatTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Vertex Data Array Format Tests") {
  tests_["Float"] = [this]() { TestF(); };
  tests_["S1"] = [this]() { TestS1(); };
  tests_["S32K"] = [this]() { TestS32K(); };
}

void VertexDataArrayFormatTests::Initialize() {
  TestSuite::Initialize();
  constexpr uint32_t kMaxAttributeSize = 4 * sizeof(float);
  attribute_buffer_ =
      MmAllocateContiguousMemoryEx(4 * kMaxAttributeSize, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
}

void VertexDataArrayFormatTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (attribute_buffer_) {
    MmFreeContiguousMemory(attribute_buffer_);
    attribute_buffer_ = nullptr;
  }
}

void VertexDataArrayFormatTests::TestF() {
  // Infinities
  static constexpr uint32_t posInf = 0x7F800000;
  static constexpr uint32_t negInf = 0xFF800000;
  // Quiet NaN on x86
  static constexpr uint32_t posNanQ = 0x7FC00000;
  static constexpr uint32_t negNanQ = 0xFFC00000;
  // Signalling NaN on x86
  static constexpr uint32_t posNanS = 0x7F800001;
  static constexpr uint32_t negNanS = 0xFF800001;
  // Max normal
  static constexpr uint32_t posMax = 0x7F7FFFFF;
  static constexpr uint32_t negMax = 0xFF7FFFFF;
  // Min normal
  static constexpr uint32_t posMinNorm = 0x00800000;
  static constexpr uint32_t negMinNorm = 0x80800000;
  // Max subnormal
  static constexpr uint32_t posMaxSubNorm = 0x007FFFFF;
  static constexpr uint32_t negMaxSubNorm = 0x807FFFFF;
  // Min subnormal
  static constexpr uint32_t posMinSubNorm = 0x00000001;
  static constexpr uint32_t negMinSubNorm = 0x80000001;

  std::list<TestHost::Computation> computations;
  std::list<TestHost::Results> results;

  static constexpr float kNormalTests[][4] = {
      {0.0f, 1.0f, -1.0f, 0.99999999f},
  };

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + 4,
               MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, 4) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, 16));
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + 4, VRAM_ADDR(attribute_buffer_));
  pb_end(p);

  char buffer[64] = {0};
  for (const auto& test : kNormalTests) {
    snprintf(buffer, sizeof(buffer), "%f,%f,%f,%f", test[0], test[1], test[2], test[3]);
    auto prepare = [this, test](const std::shared_ptr<VertexShaderProgram>& shader) {
      GenerateFloat(attribute_buffer_, test[0], test[1], test[2], test[3]);
    };
    results.emplace_back(buffer);
    computations.push_back({kShader, sizeof(kShader), prepare, &results.back()});
  }

  struct ExceptionalTest {
    const char* name;
    uint32_t values[4];
  };

  static constexpr ExceptionalTest kExceptionalTests[] = {
      {"Inf, -Inf, NaN, -Nan", {posInf, negInf, posNanQ, negNanQ}},
      {"Max, -Max, Min, -Min", {posMax, negMax, posMinNorm, negMinNorm}},
      {"MaxSub, -MaxSub, MinSub, -MinSub", {posMaxSubNorm, negMaxSubNorm, posMinSubNorm, negMinSubNorm}},
  };

  for (const auto& test : kExceptionalTests) {
    auto prepare = [this, test](const std::shared_ptr<VertexShaderProgram>& shader) {
      GenerateDWORD(attribute_buffer_, test.values[0], test.values[1], test.values[2], test.values[3]);
    };
    results.emplace_back(test.name);
    computations.push_back({kShader, sizeof(kShader), prepare, &results.back()});
  }

  host_.ComputeWithVertexBuffer(computations);
  host_.DrawResults(results, allow_saving_, output_dir_, "Float");
}

void VertexDataArrayFormatTests::TestS1() {
  std::list<TestHost::Computation> computations;
  std::list<TestHost::Results> results;

  static constexpr short kTests[][4] = {
      {32767, -32768, 256, -256},
      {1, 0, -1, -32767},
  };

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + 4,
               MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_S1) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, 4) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, 8));
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + 4, VRAM_ADDR(attribute_buffer_));
  pb_end(p);

  char buffer[64] = {0};
  for (const auto& test : kTests) {
    snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d", test[0], test[1], test[2], test[3]);
    auto prepare = [this, test](const std::shared_ptr<VertexShaderProgram>& shader) {
      GenerateShort(attribute_buffer_, test[0], test[1], test[2], test[3]);
    };
    results.emplace_back(buffer);
    computations.push_back({kShader, sizeof(kShader), prepare, &results.back()});
  }

  host_.ComputeWithVertexBuffer(computations);
  host_.DrawResults(results, allow_saving_, output_dir_, "S1");
}

void VertexDataArrayFormatTests::TestS32K() {
  std::list<TestHost::Computation> computations;
  std::list<TestHost::Results> results;

  static constexpr short kTests[][4] = {
      {32767, -32768, 256, -256},
      {1, 0, -1, -32767},
  };

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + 4,
               MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_S32K) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, 4) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, 8));
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + 4, VRAM_ADDR(attribute_buffer_));
  pb_end(p);

  char buffer[64] = {0};
  for (const auto& test : kTests) {
    snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d", test[0], test[1], test[2], test[3]);
    auto prepare = [this, test](const std::shared_ptr<VertexShaderProgram>& shader) {
      GenerateShort(attribute_buffer_, test[0], test[1], test[2], test[3]);
    };
    results.emplace_back(buffer);
    computations.push_back({kShader, sizeof(kShader), prepare, &results.back()});
  }

  host_.ComputeWithVertexBuffer(computations);
  host_.DrawResults(results, allow_saving_, output_dir_, "S32K");
}

static void GenerateFloat(void* out, float a, float b, float c, float d) {
  auto buf = reinterpret_cast<float*>(out);
  for (uint32_t i = 0; i < 4; ++i) {
    *buf++ = a;
    *buf++ = b;
    *buf++ = c;
    *buf++ = d;
  }
}

static void GenerateShort(void* out, short a, short b, short c, short d) {
  auto buf = reinterpret_cast<short*>(out);
  for (uint32_t i = 0; i < 4; ++i) {
    *buf++ = a;
    *buf++ = b;
    *buf++ = c;
    *buf++ = d;
  }
}

static void GenerateDWORD(void* out, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  auto buf = reinterpret_cast<uint32_t*>(out);
  for (uint32_t i = 0; i < 4; ++i) {
    *buf++ = a;
    *buf++ = b;
    *buf++ = c;
    *buf++ = d;
  }
}