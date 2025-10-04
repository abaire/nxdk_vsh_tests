#include "test_host.h"

// clang format off
#define _USE_MATH_DEFINES
#include <cmath>
// clang format on

#include <fpng/src/fpng.h>
#include <printf/printf.h>
#include <strings.h>
#include <windows.h>

#include <algorithm>
#include <utility>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "pgraph_diff_token.h"
#include "pushbuffer.h"
#include "shaders/vertex_shader_program.h"
#include "text_overlay.h"

// clang format off
static const uint32_t kClearStateShader[] = {
#include "shaders/clear_state.vshinc"
};

static const uint32_t kComputeFooter[] = {
#include "shaders/compute_footer.vshinc"
};
// clang format on

#define TO_BGRA(float_vals)                                                                      \
  (((uint32_t)((float_vals)[3] * 255.0f) << 24) + ((uint32_t)((float_vals)[0] * 255.0f) << 16) + \
   ((uint32_t)((float_vals)[1] * 255.0f) << 8) + ((uint32_t)((float_vals)[2] * 255.0f)))

#define MAX_FILE_PATH_SIZE 248

// LOG_GET_CONSTANT
#ifdef LOG_GET_CONSTANT
#define GET_CONSTANT(var, idx)                                                                                      \
  do {                                                                                                              \
    fetch_constant(var, idx);                                                                                       \
    PrintMsg("c[%d]: 0x%X (%f), 0x%X (%f), 0x%X (%f), 0x%X (%f)\n", (idx), *(uint32_t *)&(var)[0], (var)[0],        \
             *(uint32_t *)&(var)[1], (var)[1], *(uint32_t *)&(var)[2], (var)[2], *(uint32_t *)&(var)[3], (var)[3]); \
  } while (0)
#else
#define GET_CONSTANT(var, idx) fetch_constant(var, idx)
#endif

static void SetSurfaceFormat() {
  uint32_t value = SET_MASK(NV097_SET_SURFACE_FORMAT_COLOR, NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8) |
                   SET_MASK(NV097_SET_SURFACE_FORMAT_ZETA, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8) |
                   SET_MASK(NV097_SET_SURFACE_FORMAT_ANTI_ALIASING, NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_CENTER_1) |
                   SET_MASK(NV097_SET_SURFACE_FORMAT_TYPE, NV097_SET_SURFACE_FORMAT_TYPE_PITCH);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  p = pb_push1(p, NV097_SET_SURFACE_FORMAT, value);
  p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, (kFramebufferWidth << 16));
  p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, (kFramebufferHeight << 16));

  p = pb_push1f(p, NV097_SET_CLIP_MIN, 0.0f);
  p = pb_push1f(p, NV097_SET_CLIP_MAX, static_cast<float>(0x00FFFFFF));

  p = pb_push1(p, NV097_SET_CONTROL0, MASK(NV097_SET_CONTROL0_Z_FORMAT, NV097_SET_CONTROL0_Z_FORMAT_FIXED));

  p = pb_push1(p, NV097_SET_BLEND_ENABLE, false);
  p = pb_push1(p, NV097_SET_BLEND_EQUATION, NV097_SET_BLEND_EQUATION_V_FUNC_ADD);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_DFACTOR_V_ZERO);

  p = pb_push1(p, NV097_SET_SHADER_STAGE_PROGRAM,
               MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE0, 0) | MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE1, 0) |
                   MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE2, 0) | MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE3, 0));
  p = pb_push1(p, NV097_SET_SHADER_OTHER_STAGE_INPUT,
               MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE1, 0) | MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE2, 0) |
                   MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE3, 0));

  {
    uint32_t address = NV097_SET_TEXTURE_ADDRESS;
    uint32_t control = NV097_SET_TEXTURE_CONTROL0;
    uint32_t filter = NV097_SET_TEXTURE_FILTER;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);
  }

  p = pb_push1(p, NV097_SET_FOG_ENABLE, false);
  p = pb_push4(p, NV097_SET_TEXTURE_MATRIX_ENABLE, 0, 0, 0, 0);

  p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CW);
  p = pb_push1(p, NV097_SET_CULL_FACE, NV097_SET_CULL_FACE_V_BACK);
  p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, true);

  p = pb_push1(p, NV097_SET_COLOR_MASK,
               NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                   NV097_SET_COLOR_MASK_RED_WRITE_ENABLE | NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);

  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, true);

  p = pb_push1(p, NV097_SET_NORMALIZATION_ENABLE, false);

  pb_end(p);
}

TestHost::TestHost() {
  SetSurfaceFormat();

  uint32_t buffer_size = kFramebufferWidth * kFramebufferHeight * 4;
  compute_buffer_ = static_cast<uint8_t *>(
      MmAllocateContiguousMemoryEx(buffer_size, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  ASSERT(compute_buffer_ && "Failed to allocate compute buffer.");

  // Allocate a quad for calculations that choose to use data arrays.
  {
    constexpr uint32_t kVertexSize = 4 * sizeof(float);
    vertex_buffer_ = reinterpret_cast<float *>(
        MmAllocateContiguousMemoryEx(4 * kVertexSize, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
    uint32_t i = 0;
    vertex_buffer_[i++] = 0.0f;
    vertex_buffer_[i++] = 0.0f;
    vertex_buffer_[i++] = 1.0f;
    vertex_buffer_[i++] = 1.0f;

    vertex_buffer_[i++] = 10.0f;
    vertex_buffer_[i++] = 0.0f;
    vertex_buffer_[i++] = 1.0f;
    vertex_buffer_[i++] = 1.0f;

    vertex_buffer_[i++] = 10.0f;
    vertex_buffer_[i++] = 10.0f;
    vertex_buffer_[i++] = 1.0f;
    vertex_buffer_[i++] = 1.0f;

    vertex_buffer_[i++] = 0.0f;
    vertex_buffer_[i++] = 10.0f;
    vertex_buffer_[i++] = 1.0f;
    vertex_buffer_[i++] = 1.0f;

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT,
                 MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F) |
                     MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, 4) |
                     MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, 16));
    p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET, VRAM_ADDR(vertex_buffer_));
    pb_end(p);
  }

}

TestHost::~TestHost() {
  delete[] shader_code_;
  if (compute_buffer_) {
    MmFreeContiguousMemory(compute_buffer_);
  }
  if (vertex_buffer_) {
    MmFreeContiguousMemory(vertex_buffer_);
  }
}

void TestHost::ClearDepthStencilRegion(uint32_t depth_value, uint8_t stencil_value, uint32_t left, uint32_t top,
                                       uint32_t width, uint32_t height) const {
  if (!width || width > kFramebufferWidth) {
    width = kFramebufferWidth;
  }
  if (!height || height > kFramebufferHeight) {
    height = kFramebufferHeight;
  }

  pb_set_depth_stencil_buffer_region(NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, depth_value, stencil_value, left, top, width,
                                  height);
}

void TestHost::ClearColorRegion(uint32_t argb, uint32_t left, uint32_t top, uint32_t width, uint32_t height) const {
  if (!width || width > kFramebufferWidth) {
    width = kFramebufferWidth;
  }
  if (!height || height > kFramebufferHeight) {
    height = kFramebufferHeight;
  }
  pb_fill(static_cast<int>(left), static_cast<int>(top), static_cast<int>(width), static_cast<int>(height), argb);
}

void TestHost::Clear(uint32_t argb, uint32_t depth_value, uint8_t stencil_value) const {
  ClearColorRegion(argb);
  ClearDepthStencilRegion(depth_value, stencil_value);
  pb_erase_text_screen();
  TextOverlay::Reset();
}

static void fetch_constant(float *buffer, uint32_t index) {
  pb_wait_until_gr_not_busy();

  // See RDI dumping code in nv2a-trace.
  // https://github.com/XboxDev/nv2a-trace/blob/65bdd2369a5b216cfc47c9545f870c49d118276b/Trace.py#L58
  static constexpr uint32_t VP_CONSTANTS_BASE = 0x170000;

  VIDEOREG(NV_PGRAPH_RDI_INDEX) = VP_CONSTANTS_BASE + index * 16;
  for (uint32_t component = 0; component < 4; ++component) {
    auto value = VIDEOREG(NV_PGRAPH_RDI_DATA);
    buffer[3 - component] = *(float *)&value;
  }
}

static void fetch_results(TestHost::Results &results) {
  for (uint32_t i = 0; i < 32; ++i) {
    if (results.results_mask & (1 << i)) {
      GET_CONSTANT(results.cOut[i], kOutputConstantBaseIndex + i);
    }
  }
}

void TestHost::Compute(const std::list<Computation> &computations) {
  static constexpr float kPatchSize = 16.0f;

  for (auto &comp : computations) {
    auto shader = PrepareCalculation(comp.shader_code, comp.shader_size);
    if (comp.prepare) {
      comp.prepare(shader);
    }
    shader->PrepareDraw();

    while (pb_busy()) {
    }

    if (comp.draw) {
      comp.draw();
    } else {
      float left = 0.0f;
      float top = 0.0f;

      Begin(PRIMITIVE_QUADS);
      SetVertex(left, 0.0, 0.0, 1.0);
      SetVertex(left + kPatchSize, top, 0.0, 1.0);
      SetVertex(left + kPatchSize, top + kPatchSize, 0.0, 1.0);
      SetVertex(left, top + kPatchSize, 0.0, 1.0);
      End();

      left += kPatchSize;
      Begin(PRIMITIVE_QUADS);
      SetVertex(left, 0.0, 0.0, 1.0);
      SetVertex(left + kPatchSize, top, 0.0, 1.0);
      SetVertex(left + kPatchSize, top + kPatchSize, 0.0, 1.0);
      SetVertex(left, top + kPatchSize, 0.0, 1.0);
      End();
    }

    // Force inputs to be reloaded, may not be necessary for immediate mode commands.
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();

    while (pb_busy()) {
    }

    fetch_results(*comp.results);
  }
}

void TestHost::ComputeWithVertexBuffer(const std::list<Computation> &computations) {
  for (auto &comp : computations) {
    assert(!comp.draw && "ComputeWithVertexBuffer must not be called with a draw override.");
    PrintMsg("Prepare calc in ComputeWithVertexBuffer\n");
    auto shader = PrepareCalculation(comp.shader_code, comp.shader_size);
    if (comp.prepare) {
      comp.prepare(shader);
    }
    shader->PrepareDraw();

    while (pb_busy()) {
    }

    auto p = pb_begin();
    // Force inputs to be reloaded.
    p = pb_push1(p, NV097_BREAK_VERTEX_BUFFER_CACHE, 0);

    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_QUADS);
    p = pb_push1(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                 MASK(NV097_DRAW_ARRAYS_COUNT, 3) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 0));
    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

    // Stall for output.
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    while (pb_busy()) {
    }

    fetch_results(*comp.results);
  }
}

void TestHost::DrawResults(const std::list<Results> &results, bool allow_saving, const std::string &output_directory,
                           const std::string &name) {
  pb_wait_for_vbl();
  pb_reset();

  Clear(0x2F2C2E);

  auto shader = vertex_shader_program_;

  TextOverlay::Print("%s\n", name.c_str());

  auto print_vals = [](uint32_t index, const XboxMath::vector_t vals,
                       const std::map<uint32_t, std::string> &labels) {
    // pbkit's handling of exceptional floats is not trustable.
    char buf[128] = {0};
    auto label = labels.find(index);
    if (label == labels.end()) {
      snprintf_(buf, sizeof(buf), " [%d]:%f,%f,%f,%f\n", index, vals[0], vals[1], vals[2], vals[3]);
    } else {
      snprintf_(buf, sizeof(buf), " %s%f,%f,%f,%f\n", label->second.c_str(), vals[0], vals[1], vals[2], vals[3]);
    }
    TextOverlay::Print("%s", buf);
  };

  for (auto &result : results) {
    TextOverlay::Print("%s:\n", result.title.c_str());
    for (uint32_t i = 0; i < 32; ++i) {
      if (result.results_mask & (1 << i)) {
        print_vals(i, result.cOut[i], result.result_labels);
      }
    }
  }

  bool perform_save = allow_saving && save_results_;
  if (!perform_save) {
    TextOverlay::PrintAt(0, 55, (char *)"ns");
  }

  TextOverlay::Render();

  if (perform_save) {
    // TODO: See why waiting for tiles to be non-busy results in the screen not updating anymore.
    // In theory this should wait for all tiles to be rendered before capturing.
    pb_wait_for_vbl();

    SaveBackBuffer(output_directory, name);
  }

  while (pb_finished()) {
  }

  SetVertexShaderProgram(shader);
}

std::shared_ptr<VertexShaderProgram> TestHost::PrepareCalculation(const uint32_t *shader_code, uint32_t shader_size) {
  ASSERT(shader_size >= 4);
  if (shader_code_) {
    delete[] shader_code_;
  }

  shader_code_size_ = shader_size + sizeof(kComputeFooter);
  shader_code_ = new uint32_t[shader_code_size_];

  memcpy(shader_code_, shader_code, shader_size);

  uint32_t end_offset = shader_size / 4;

  // Clear the "end" bit on the shader code.
  shader_code_[end_offset - 1] &= ~0x01;

  memcpy(shader_code_ + end_offset, kComputeFooter, sizeof(kComputeFooter));

  auto shader = std::make_shared<VertexShaderProgram>();
  shader->SetShaderOverride(shader_code_, shader_code_size_);
  SetVertexShaderProgram(shader);

  return shader;
}

void TestHost::SaveBackBuffer(const std::string &output_directory, const std::string &name) {
  auto target_file = PrepareSaveFile(output_directory, name);

  auto buffer = pb_agp_access(pb_back_buffer());
  auto width = static_cast<int>(pb_back_buffer_width());
  auto height = static_cast<int>(pb_back_buffer_height());
  auto pitch = static_cast<int>(pb_back_buffer_pitch());

  // FIXME: Support 16bpp surfaces
  ASSERT((pitch == width * 4) && "Expected packed 32bpp surface");

  // Swizzle color channels ARGB -> OBGR
  unsigned int num_pixels = width * height;
  uint32_t *pre_enc_buf = (uint32_t *)malloc(num_pixels * 4);
  ASSERT(pre_enc_buf && "Failed to allocate pre-encode buffer");
  uint32_t full_alpha = 0xFF000000;
  for (unsigned int i = 0; i < num_pixels; i++) {
    uint32_t c = static_cast<uint32_t *>(buffer)[i];
    pre_enc_buf[i] = (c & 0xff00ff00) | ((c >> 16) & 0xff) | ((c & 0xff) << 16) | full_alpha;
  }

  std::vector<uint8_t> out_buf;
  if (!fpng::fpng_encode_image_to_memory((void *)pre_enc_buf, width, height, 4, out_buf)) {
    ASSERT(!"Failed to encode PNG image");
  }
  free(pre_enc_buf);

  FILE *pFile = fopen(target_file.c_str(), "wb");
  ASSERT(pFile && "Failed to open output PNG image");
  if (fwrite(out_buf.data(), 1, out_buf.size(), pFile) != out_buf.size()) {
    ASSERT(!"Failed to write output PNG image");
  }
  if (fclose(pFile)) {
    ASSERT(!"Failed to close output PNG image");
  }
}

void TestHost::Begin(DrawPrimitive primitive) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, primitive);
  pb_end(p);
}

void TestHost::End() const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  pb_end(p);
}

void TestHost::SetVertex(float x, float y, float z) const {
  auto p = pb_begin();
  p = pb_push3f(p, NV097_SET_VERTEX3F, x, y, z);
  pb_end(p);
}

void TestHost::SetVertex(float x, float y, float z, float w) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_VERTEX4F, x, y, z, w);
  pb_end(p);
}

void TestHost::SetWeight(float w1, float w2, float w3, float w4) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_WEIGHT4F, w1, w2, w3, w4);
  pb_end(p);
}

void TestHost::SetWeight(float w) const {
  auto p = pb_begin();
  p = pb_push1f(p, NV097_SET_WEIGHT1F, w);
  pb_end(p);
}

void TestHost::SetNormal(float x, float y, float z) const {
  auto p = pb_begin();
  p = pb_push3(p, NV097_SET_NORMAL3F, *(uint32_t *)&x, *(uint32_t *)&y, *(uint32_t *)&z);
  pb_end(p);
}

void TestHost::SetNormal3S(int x, int y, int z) const {
  auto p = pb_begin();
  uint32_t xy = (x & 0xFFFF) | y << 16;
  uint32_t z0 = z & 0xFFFF;
  p = pb_push2(p, NV097_SET_NORMAL3S, xy, z0);
  pb_end(p);
}

void TestHost::SetDiffuse(float r, float g, float b, float a) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_DIFFUSE_COLOR4F, r, g, b, a);
  pb_end(p);
}

void TestHost::SetDiffuse(float r, float g, float b) const {
  auto p = pb_begin();
  p = pb_push3f(p, NV097_SET_DIFFUSE_COLOR3F, r, g, b);
  pb_end(p);
}

void TestHost::SetDiffuse(uint32_t color) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DIFFUSE_COLOR4I, color);
  pb_end(p);
}

void TestHost::SetSpecular(float r, float g, float b, float a) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_SPECULAR_COLOR4F, r, g, b, a);
  pb_end(p);
}

void TestHost::SetSpecular(float r, float g, float b) const {
  auto p = pb_begin();
  p = pb_push3f(p, NV097_SET_SPECULAR_COLOR3F, r, g, b);
  pb_end(p);
}

void TestHost::SetSpecular(uint32_t color) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SPECULAR_COLOR4I, color);
  pb_end(p);
}

void TestHost::SetFogCoord(float fc) const {
  auto p = pb_begin();
  p = pb_push1f(p, NV097_SET_FOG_COORD, fc);
  pb_end(p);
}

void TestHost::SetPointSize(float ps) const {
  auto p = pb_begin();
  p = pb_push1f(p, NV097_SET_POINT_SIZE, ps);
  pb_end(p);
}

void TestHost::SetTexCoord0(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEXCOORD0_2F, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord0S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD0_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord0(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD0_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord0S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD0_4S, st, pq);
  pb_end(pb);
}

void TestHost::SetTexCoord1(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEXCOORD1_2F, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord1S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD1_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord1(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD1_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord1S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD1_4S, st, pq);
  pb_end(pb);
}

void TestHost::SetTexCoord2(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2f(p, NV097_SET_TEXCOORD2_2F, u, v);
  pb_end(p);
}

void TestHost::SetTexCoord2S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD2_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord2(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD2_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord2S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD2_4S, st, pq);
  pb_end(pb);
}

void TestHost::SetTexCoord3(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEXCOORD3_2F, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord3S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD3_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord3(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD3_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord3S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD3_4S, st, pq);
  pb_end(pb);
}

void TestHost::EnsureFolderExists(const std::string &folder_path) {
  if (folder_path.length() > MAX_FILE_PATH_SIZE) {
    ASSERT(!"Folder Path is too long.");
  }

  char buffer[MAX_FILE_PATH_SIZE + 1] = {0};
  const char *path_start = folder_path.c_str();
  const char *slash = strchr(path_start, '\\');
  slash = strchr(slash + 1, '\\');

  while (slash) {
    strncpy(buffer, path_start, slash - path_start);
    if (!CreateDirectory(buffer, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
      ASSERT(!"Failed to create output directory.");
    }

    slash = strchr(slash + 1, '\\');
  }

  // Handle case where there was no trailing slash.
  if (!CreateDirectory(path_start, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
    ASSERT(!"Failed to create output directory.");
  }
}

// Returns the full output filepath including the filename
// Creates output directory if it does not exist
std::string TestHost::PrepareSaveFile(std::string output_directory, const std::string &filename,
                                      const std::string &extension) {
  EnsureFolderExists(output_directory);

  output_directory += "\\";
  output_directory += filename;
  output_directory += extension;

  if (output_directory.length() > MAX_FILE_PATH_SIZE) {
    ASSERT(!"Full save file path is too long.");
  }

  return output_directory;
}

void TestHost::SetVertexShaderProgram(std::shared_ptr<VertexShaderProgram> program) {
  vertex_shader_program_ = std::move(program);

  if (vertex_shader_program_) {
    vertex_shader_program_->Activate();
  } else {
    auto p = pb_begin();
    p = pb_push1(
        p, NV097_SET_TRANSFORM_EXECUTION_MODE,
        MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_FIXED) |
            MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0x0);
    p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 0x0);
    pb_end(p);
  }
}

void TestHost::SetCombinerControl(int num_combiners, bool same_factor0, bool same_factor1, bool mux_msb) const {
  ASSERT(num_combiners > 0 && num_combiners < 8);
  uint32_t setting = MASK(NV097_SET_COMBINER_CONTROL_ITERATION_COUNT, num_combiners);
  if (!same_factor0) {
    setting |= MASK(NV097_SET_COMBINER_CONTROL_FACTOR0, NV097_SET_COMBINER_CONTROL_FACTOR0_EACH_STAGE);
  }
  if (!same_factor1) {
    setting |= MASK(NV097_SET_COMBINER_CONTROL_FACTOR1, NV097_SET_COMBINER_CONTROL_FACTOR1_EACH_STAGE);
  }
  if (mux_msb) {
    setting |= MASK(NV097_SET_COMBINER_CONTROL_MUX_SELECT, NV097_SET_COMBINER_CONTROL_MUX_SELECT_MSB);
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_CONTROL, setting);
  pb_end(p);
}

void TestHost::SetInputColorCombiner(int combiner, CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping,
                                     CombinerSource b_source, bool b_alpha, CombinerMapping b_mapping,
                                     CombinerSource c_source, bool c_alpha, CombinerMapping c_mapping,
                                     CombinerSource d_source, bool d_alpha, CombinerMapping d_mapping) const {
  uint32_t value = MakeInputCombiner(a_source, a_alpha, a_mapping, b_source, b_alpha, b_mapping, c_source, c_alpha,
                                     c_mapping, d_source, d_alpha, d_mapping);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_ICW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearInputColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_ICW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearInputColorCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_ICW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

void TestHost::SetInputAlphaCombiner(int combiner, CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping,
                                     CombinerSource b_source, bool b_alpha, CombinerMapping b_mapping,
                                     CombinerSource c_source, bool c_alpha, CombinerMapping c_mapping,
                                     CombinerSource d_source, bool d_alpha, CombinerMapping d_mapping) const {
  uint32_t value = MakeInputCombiner(a_source, a_alpha, a_mapping, b_source, b_alpha, b_mapping, c_source, c_alpha,
                                     c_mapping, d_source, d_alpha, d_mapping);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_ICW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearInputAlphaColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_ICW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearInputAlphaCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_ICW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

uint32_t TestHost::MakeInputCombiner(CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping,
                                     CombinerSource b_source, bool b_alpha, CombinerMapping b_mapping,
                                     CombinerSource c_source, bool c_alpha, CombinerMapping c_mapping,
                                     CombinerSource d_source, bool d_alpha, CombinerMapping d_mapping) const {
  auto channel = [](CombinerSource src, bool alpha, CombinerMapping mapping) {
    return src + (alpha << 4) + (mapping << 5);
  };

  uint32_t ret = (channel(a_source, a_alpha, a_mapping) << 24) + (channel(b_source, b_alpha, b_mapping) << 16) +
                 (channel(c_source, c_alpha, c_mapping) << 8) + channel(d_source, d_alpha, d_mapping);
  return ret;
}

void TestHost::SetOutputColorCombiner(int combiner, TestHost::CombinerDest ab_dst, TestHost::CombinerDest cd_dst,
                                      TestHost::CombinerDest sum_dst, bool ab_dot_product, bool cd_dot_product,
                                      TestHost::CombinerSumMuxMode sum_or_mux, TestHost::CombinerOutOp op,
                                      bool alpha_from_ab_blue, bool alpha_from_cd_blue) const {
  uint32_t value = MakeOutputCombiner(ab_dst, cd_dst, sum_dst, ab_dot_product, cd_dot_product, sum_or_mux, op);
  if (alpha_from_ab_blue) {
    value |= (1 << 19);
  }
  if (alpha_from_cd_blue) {
    value |= (1 << 18);
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_OCW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearOutputColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_OCW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearOutputColorCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_OCW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

void TestHost::SetOutputAlphaCombiner(int combiner, CombinerDest ab_dst, CombinerDest cd_dst, CombinerDest sum_dst,
                                      bool ab_dot_product, bool cd_dot_product, CombinerSumMuxMode sum_or_mux,
                                      CombinerOutOp op) const {
  uint32_t value = MakeOutputCombiner(ab_dst, cd_dst, sum_dst, ab_dot_product, cd_dot_product, sum_or_mux, op);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_OCW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearOutputAlphaColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_OCW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearOutputAlphaCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_OCW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

uint32_t TestHost::MakeOutputCombiner(TestHost::CombinerDest ab_dst, TestHost::CombinerDest cd_dst,
                                      TestHost::CombinerDest sum_dst, bool ab_dot_product, bool cd_dot_product,
                                      TestHost::CombinerSumMuxMode sum_or_mux, TestHost::CombinerOutOp op) const {
  uint32_t ret = cd_dst | (ab_dst << 4) | (sum_dst << 8);
  if (cd_dot_product) {
    ret |= 1 << 12;
  }
  if (ab_dot_product) {
    ret |= 1 << 13;
  }
  if (sum_or_mux) {
    ret |= 1 << 14;
  }
  ret |= op << 15;

  return ret;
}

void TestHost::SetFinalCombiner0(TestHost::CombinerSource a_source, bool a_alpha, bool a_invert,
                                 TestHost::CombinerSource b_source, bool b_alpha, bool b_invert,
                                 TestHost::CombinerSource c_source, bool c_alpha, bool c_invert,
                                 TestHost::CombinerSource d_source, bool d_alpha, bool d_invert) const {
  auto channel = [](CombinerSource src, bool alpha, bool invert) { return src + (alpha << 4) + (invert << 5); };

  uint32_t value = (channel(a_source, a_alpha, a_invert) << 24) + (channel(b_source, b_alpha, b_invert) << 16) +
                   (channel(c_source, c_alpha, c_invert) << 8) + channel(d_source, d_alpha, d_invert);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, value);
  pb_end(p);
}

void TestHost::SetFinalCombiner1(TestHost::CombinerSource e_source, bool e_alpha, bool e_invert,
                                 TestHost::CombinerSource f_source, bool f_alpha, bool f_invert,
                                 TestHost::CombinerSource g_source, bool g_alpha, bool g_invert,
                                 bool specular_add_invert_r0, bool specular_add_invert_v1, bool specular_clamp) const {
  auto channel = [](CombinerSource src, bool alpha, bool invert) { return src + (alpha << 4) + (invert << 5); };

  // The V1+R0 sum is not available in CW1.
  ASSERT(e_source != SRC_SPEC_R0_SUM && f_source != SRC_SPEC_R0_SUM && g_source != SRC_SPEC_R0_SUM);

  uint32_t value = (channel(e_source, e_alpha, e_invert) << 24) + (channel(f_source, f_alpha, f_invert) << 16) +
                   (channel(g_source, g_alpha, g_invert) << 8);
  if (specular_add_invert_r0) {
    // NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_ADD_INVERT_R12 crashes on hardware.
    value += (1 << 5);
  }
  if (specular_add_invert_v1) {
    value += NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_ADD_INVERT_R5;
  }
  if (specular_clamp) {
    value += NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_CLAMP;
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1, value);
  pb_end(p);
}

void TestHost::SetCombinerFactorC0(int combiner, uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_FACTOR0 + 4 * combiner, value);
  pb_end(p);
}

void TestHost::SetCombinerFactorC0(int combiner, float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetCombinerFactorC0(combiner, TO_BGRA(rgba));
}

void TestHost::SetCombinerFactorC1(int combiner, uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_FACTOR1 + 4 * combiner, value);
  pb_end(p);
}

void TestHost::SetCombinerFactorC1(int combiner, float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetCombinerFactorC1(combiner, TO_BGRA(rgba));
}

void TestHost::SetFinalCombinerFactorC0(uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SPECULAR_FOG_FACTOR, value);
  pb_end(p);
}

void TestHost::SetFinalCombinerFactorC0(float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetFinalCombinerFactorC0(TO_BGRA(rgba));
}

void TestHost::SetFinalCombinerFactorC1(uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SPECULAR_FOG_FACTOR + 0x04, value);
  pb_end(p);
}

void TestHost::SetFinalCombinerFactorC1(float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetFinalCombinerFactorC1(TO_BGRA(rgba));
}

void TestHost::ClearState() {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x20001);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_OFF);
  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);

  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_LIGHT_MODEL_TWO_SIDE_ENABLE, 0);
  p = pb_push1(p, NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  p = pb_push1(p, NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x10, 0);           // Specular
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x1C, 0xFFFFFFFF);  // Back diffuse
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x20, 0);           // Back specular

  p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, false);
  p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, false);
  p = pb_push1(p, NV097_SET_POINT_SIZE, 8);

  p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, 0);
  pb_end(p);

  auto current_shader = GetShaderProgram();

  static std::shared_ptr<VertexShaderProgram> shader;
  if (!shader) {
    shader = std::make_shared<VertexShaderProgram>();
    shader->SetShaderOverride(kClearStateShader, sizeof(kClearStateShader));

    // Clear the outputs registers.
    shader->SetUniformF(188, 0.0f, 0.0f, 0.0f, 0.0f);
    shader->SetUniformF(189, 0.0f, 0.0f, 0.0f, 0.0f);
    shader->SetUniformF(190, 0.0f, 0.0f, 0.0f, 0.0f);
    shader->SetUniformF(191, 0.0f, 0.0f, 0.0f, 0.0f);
  }
  SetVertexShaderProgram(shader);

  // Render multiple times to ensure any parallelized hardware is initialized.
  for (uint32_t i = 0; i < 16; ++i) {
    Begin(PRIMITIVE_QUADS);
    SetDiffuse(0);
    SetVertex(kFramebufferWidth, 0.0, 0.0, 1.0);
    SetVertex(kFramebufferWidth, kFramebufferHeight, 0.0, 1.0);
    SetVertex(0.0, kFramebufferHeight, 0.0, 1.0);
    SetVertex(0.0, 0.0, 0.0, 1.0);
    End();
  }

  SetVertexShaderProgram(current_shader);

  SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_R0), TestHost::OneInput(),
                        TestHost::ColorInput(TestHost::SRC_R1), TestHost::OneInput());
  SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_R1);

  SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_R0), TestHost::OneInput(),
                        TestHost::AlphaInput(TestHost::SRC_R1), TestHost::OneInput());
  SetOutputAlphaCombiner(0, TestHost::DST_R0, TestHost::DST_R1);
}
