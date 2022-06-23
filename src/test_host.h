#ifndef NXDK_PGRAPH_TESTS_TEST_HOST_H
#define NXDK_PGRAPH_TESTS_TEST_HOST_H

#include <pbkit/pbkit.h>

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "math3d.h"
#include "nxdk_ext.h"
#include "string"

class VertexShaderProgram;

constexpr uint32_t kFramebufferWidth = 640;
constexpr uint32_t kFramebufferHeight = 480;
constexpr uint32_t kFramebufferPitch = kFramebufferWidth * 4;

// The first pgraph 0x3D subchannel that can be used by tests.
// It appears that this must be exactly one more than the last subchannel configured by pbkit or it will trigger an
// exception in xemu.
constexpr uint32_t kNextSubchannel = NEXT_SUBCH;
// The first pgraph context channel that can be used by tests.
constexpr int32_t kNextContextChannel = 25;

constexpr uint32_t kNoStrideOverride = 0xFFFFFFFF;

#define ABGR(x) (((x)&0xFF00FF00) | (((x)&0xFF) << 16) | (((x)&0x00FF0000) >> 16))

#define VRAM_ADDR(x) (reinterpret_cast<uint32_t>(x) & 0x03FFFFFF)
#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// Defines which fields in a TestHost::Results should be displayed.
constexpr uint32_t RES_0 = 1 << 0;
constexpr uint32_t RES_1 = 1 << 1;
constexpr uint32_t RES_2 = 1 << 2;
constexpr uint32_t RES_3 = 1 << 3;
constexpr uint32_t RES_ALL = RES_0 | RES_1 | RES_2 | RES_3;

class TestHost {
 public:
  struct Results {
    std::string title;
    uint32_t results_mask;
    VECTOR c188{0.0f, 0.0f, 0.0f, 0.0f};
    VECTOR c189{0.0f, 0.0f, 0.0f, 0.0f};
    VECTOR c190{0.0f, 0.0f, 0.0f, 0.0f};
    VECTOR c191{0.0f, 0.0f, 0.0f, 0.0f};

    std::map<uint32_t, std::string> result_labels;

    explicit Results(std::string title, uint32_t results_mask = RES_0,
                     std::map<uint32_t, std::string> result_labels = {})
        : title(std::move(title)), results_mask(results_mask), result_labels(std::move(result_labels)) {}
  };

  struct Computation {
    const uint32_t *shader_code{nullptr};
    uint32_t shader_size{0};

    std::function<void(const std::shared_ptr<VertexShaderProgram> &)> prepare;

    Results *results;
  };

  enum DrawPrimitive {
    PRIMITIVE_POINTS = NV097_SET_BEGIN_END_OP_POINTS,
    PRIMITIVE_LINES = NV097_SET_BEGIN_END_OP_LINES,
    PRIMITIVE_LINE_LOOP = NV097_SET_BEGIN_END_OP_LINE_LOOP,
    PRIMITIVE_LINE_STRIP = NV097_SET_BEGIN_END_OP_LINE_STRIP,
    PRIMITIVE_TRIANGLES = NV097_SET_BEGIN_END_OP_TRIANGLES,
    PRIMITIVE_TRIANGLE_STRIP = NV097_SET_BEGIN_END_OP_TRIANGLE_STRIP,
    PRIMITIVE_TRIANGLE_FAN = NV097_SET_BEGIN_END_OP_TRIANGLE_FAN,
    PRIMITIVE_QUADS = NV097_SET_BEGIN_END_OP_QUADS,
    PRIMITIVE_QUAD_STRIP = NV097_SET_BEGIN_END_OP_QUAD_STRIP,
    PRIMITIVE_POLYGON = NV097_SET_BEGIN_END_OP_POLYGON,
  };

  enum CombinerSource {
    SRC_ZERO = 0,     // 0
    SRC_C0,           // Constant[0]
    SRC_C1,           // Constant[1]
    SRC_FOG,          // Fog coordinate
    SRC_DIFFUSE,      // Vertex diffuse
    SRC_SPECULAR,     // Vertex specular
    SRC_6,            // ?
    SRC_7,            // ?
    SRC_TEX0,         // Texcoord0
    SRC_TEX1,         // Texcoord1
    SRC_TEX2,         // Texcoord2
    SRC_TEX3,         // Texcoord3
    SRC_R0,           // R0 from the vertex shader
    SRC_R1,           // R1 from the vertex shader
    SRC_SPEC_R0_SUM,  // Specular + R0
    SRC_EF_PROD,      // Combiner param E * F
  };

  enum CombinerDest {
    DST_DISCARD = 0,  // Discard the calculation
    DST_C0,           // Constant[0]
    DST_C1,           // Constant[1]
    DST_FOG,          // Fog coordinate
    DST_DIFFUSE,      // Vertex diffuse
    DST_SPECULAR,     // Vertex specular
    DST_6,            // ?
    DST_7,            // ?
    DST_TEX0,         // Texcoord0
    DST_TEX1,         // Texcoord1
    DST_TEX2,         // Texcoord2
    DST_TEX3,         // Texcoord3
    DST_R0,           // R0 from the vertex shader
    DST_R1,           // R1 from the vertex shader
    DST_SPEC_R0_SUM,  // Specular + R1
    DST_EF_PROD,      // Combiner param E * F
  };

  enum CombinerSumMuxMode {
    SM_SUM = 0,  // ab + cd
    SM_MUX = 1,  // r0.a is used to select cd or ab
  };

  enum CombinerOutOp {
    OP_IDENTITY = 0,           // y = x
    OP_BIAS = 1,               // y = x - 0.5
    OP_SHIFT_LEFT_1 = 2,       // y = x*2
    OP_SHIFT_LEFT_1_BIAS = 3,  // y = (x - 0.5)*2
    OP_SHIFT_LEFT_2 = 4,       // y = x*4
    OP_SHIFT_RIGHT_1 = 6,      // y = x/2
  };

  enum CombinerMapping {
    MAP_UNSIGNED_IDENTITY,  // max(0,x)         OK for final combiner
    MAP_UNSIGNED_INVERT,    // 1 - max(0,x)     OK for final combiner
    MAP_EXPAND_NORMAL,      // 2*max(0,x) - 1   invalid for final combiner
    MAP_EXPAND_NEGATE,      // 1 - 2*max(0,x)   invalid for final combiner
    MAP_HALFBIAS_NORMAL,    // max(0,x) - 1/2   invalid for final combiner
    MAP_HALFBIAS_NEGATE,    // 1/2 - max(0,x)   invalid for final combiner
    MAP_SIGNED_IDENTITY,    // x                invalid for final combiner
    MAP_SIGNED_NEGATE,      // -x               invalid for final combiner
  };

  struct CombinerInput {
    CombinerSource source;
    bool alpha;
    CombinerMapping mapping;
  };

  struct ColorInput : public CombinerInput {
    explicit ColorInput(CombinerSource s, CombinerMapping m = MAP_UNSIGNED_IDENTITY) : CombinerInput() {
      source = s;
      alpha = false;
      mapping = m;
    }
  };

  struct AlphaInput : public CombinerInput {
    explicit AlphaInput(CombinerSource s, CombinerMapping m = MAP_UNSIGNED_IDENTITY) : CombinerInput() {
      source = s;
      alpha = true;
      mapping = m;
    }
  };

  struct ZeroInput : public CombinerInput {
    explicit ZeroInput() : CombinerInput() {
      source = SRC_ZERO;
      alpha = false;
      mapping = MAP_UNSIGNED_IDENTITY;
    }
  };

  struct NegativeOneInput : public CombinerInput {
    explicit NegativeOneInput() : CombinerInput() {
      source = SRC_ZERO;
      alpha = false;
      mapping = MAP_EXPAND_NORMAL;
    }
  };

  struct OneInput : public CombinerInput {
    explicit OneInput() : CombinerInput() {
      source = SRC_ZERO;
      alpha = false;
      mapping = MAP_UNSIGNED_INVERT;
    }
  };

 public:
  TestHost();
  ~TestHost();

  static void NullPrepare(const std::shared_ptr<VertexShaderProgram> &){};

  void Compute(const std::list<Computation> &computations);
  void DrawResults(const std::list<Results> &results, bool allow_saving, const std::string &output_directory,
                   const std::string &name);

  // Run a computation whose inputs have already been specified via NV097_SET_VERTEX_DATA_ARRAY_FORMAT. The vertices
  // are assumed to be a single quad in clockwise order (indices 0, 1, 2, 3 will be used).
  void ComputeWithVertexBuffer(const std::list<Computation> &computations);

  void SetVertexShaderProgram(std::shared_ptr<VertexShaderProgram> program);
  std::shared_ptr<VertexShaderProgram> GetShaderProgram() const { return vertex_shader_program_; }

  // Clear all vertex shaders registers to known values.
  void ClearState();

  // Start the process of rendering an inline-defined primitive (specified via SetXXXX methods below).
  // Note that End() must be called to trigger rendering, and that SetVertex() triggers the creation of a vertex.
  void Begin(DrawPrimitive primitive) const;
  void End() const;

  // Trigger creation of a vertex, applying the last set attributes.
  void SetVertex(float x, float y, float z) const;
  // Trigger creation of a vertex, applying the last set attributes.
  void SetVertex(float x, float y, float z, float w) const;
  // Trigger creation of a vertex, applying the last set attributes.
  inline void SetVertex(const VECTOR pt) const { SetVertex(pt[_X], pt[_Y], pt[_Z], pt[_W]); }

  void SetWeight(float w) const;
  void SetWeight(float w1, float w2, float w3, float w4) const;
  void SetNormal(float x, float y, float z) const;
  void SetNormal3S(int x, int y, int z) const;
  void SetDiffuse(float r, float g, float b, float a) const;
  void SetDiffuse(float r, float g, float b) const;
  void SetDiffuse(uint32_t rgba) const;
  void SetSpecular(float r, float g, float b, float a) const;
  void SetSpecular(float r, float g, float b) const;
  void SetSpecular(uint32_t rgba) const;
  void SetFogCoord(float fc) const;
  void SetPointSize(float ps) const;
  void SetTexCoord0(float u, float v) const;
  void SetTexCoord0S(int u, int v) const;
  void SetTexCoord0(float s, float t, float p, float q) const;
  void SetTexCoord0S(int s, int t, int p, int q) const;
  void SetTexCoord1(float u, float v) const;
  void SetTexCoord1S(int u, int v) const;
  void SetTexCoord1(float s, float t, float p, float q) const;
  void SetTexCoord1S(int s, int t, int p, int q) const;
  void SetTexCoord2(float u, float v) const;
  void SetTexCoord2S(int u, int v) const;
  void SetTexCoord2(float s, float t, float p, float q) const;
  void SetTexCoord2S(int s, int t, int p, int q) const;
  void SetTexCoord3(float u, float v) const;
  void SetTexCoord3S(int u, int v) const;
  void SetTexCoord3(float s, float t, float p, float q) const;
  void SetTexCoord3S(int s, int t, int p, int q) const;

  bool GetSaveResults() const { return save_results_; }
  void SetSaveResults(bool enable = true) { save_results_ = enable; }

  static void EnsureFolderExists(const std::string &folder_path);

  void Clear(uint32_t argb = 0xFF000000, uint32_t depth_value = 0xFFFFFFFF, uint8_t stencil_value = 0x00) const;
  void ClearDepthStencilRegion(uint32_t depth_value, uint8_t stencil_value, uint32_t left = 0, uint32_t top = 0,
                               uint32_t width = 0, uint32_t height = 0) const;
  void ClearColorRegion(uint32_t argb, uint32_t left = 0, uint32_t top = 0, uint32_t width = 0,
                        uint32_t height = 0) const;

 private:
  std::shared_ptr<VertexShaderProgram> PrepareCalculation(const uint32_t *shader_code, uint32_t shader_size);

  void SaveBackBuffer(const std::string &output_directory, const std::string &name);

  // Sets up the number of enabled color combiners and behavior flags.
  //
  // same_factor0 == true will reuse the C0 constant across all enabled stages.
  // same_factor1 == true will reuse the C1 constant across all enabled stages.
  void SetCombinerControl(int num_combiners = 1, bool same_factor0 = false, bool same_factor1 = false,
                          bool mux_msb = false) const;

  void SetInputColorCombiner(int combiner, CombinerInput a, CombinerInput b = ZeroInput(),
                             CombinerInput c = ZeroInput(), CombinerInput d = ZeroInput()) const {
    SetInputColorCombiner(combiner, a.source, a.alpha, a.mapping, b.source, b.alpha, b.mapping, c.source, c.alpha,
                          c.mapping, d.source, d.alpha, d.mapping);
  }
  void SetInputColorCombiner(int combiner, CombinerSource a_source = SRC_ZERO, bool a_alpha = false,
                             CombinerMapping a_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource b_source = SRC_ZERO,
                             bool b_alpha = false, CombinerMapping b_mapping = MAP_UNSIGNED_IDENTITY,
                             CombinerSource c_source = SRC_ZERO, bool c_alpha = false,
                             CombinerMapping c_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource d_source = SRC_ZERO,
                             bool d_alpha = false, CombinerMapping d_mapping = MAP_UNSIGNED_IDENTITY) const;
  void ClearInputColorCombiner(int combiner) const;
  void ClearInputColorCombiners() const;

  void SetInputAlphaCombiner(int combiner, CombinerInput a, CombinerInput b = ZeroInput(),
                             CombinerInput c = ZeroInput(), CombinerInput d = ZeroInput()) const {
    SetInputAlphaCombiner(combiner, a.source, a.alpha, a.mapping, b.source, b.alpha, b.mapping, c.source, c.alpha,
                          c.mapping, d.source, d.alpha, d.mapping);
  }
  void SetInputAlphaCombiner(int combiner, CombinerSource a_source = SRC_ZERO, bool a_alpha = false,
                             CombinerMapping a_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource b_source = SRC_ZERO,
                             bool b_alpha = false, CombinerMapping b_mapping = MAP_UNSIGNED_IDENTITY,
                             CombinerSource c_source = SRC_ZERO, bool c_alpha = false,
                             CombinerMapping c_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource d_source = SRC_ZERO,
                             bool d_alpha = false, CombinerMapping d_mapping = MAP_UNSIGNED_IDENTITY) const;
  void ClearInputAlphaColorCombiner(int combiner) const;
  void ClearInputAlphaCombiners() const;

  void SetOutputColorCombiner(int combiner, CombinerDest ab_dst = DST_DISCARD, CombinerDest cd_dst = DST_DISCARD,
                              CombinerDest sum_dst = DST_DISCARD, bool ab_dot_product = false,
                              bool cd_dot_product = false, CombinerSumMuxMode sum_or_mux = SM_SUM,
                              CombinerOutOp op = OP_IDENTITY, bool alpha_from_ab_blue = false,
                              bool alpha_from_cd_blue = false) const;
  void ClearOutputColorCombiner(int combiner) const;
  void ClearOutputColorCombiners() const;

  void SetOutputAlphaCombiner(int combiner, CombinerDest ab_dst = DST_DISCARD, CombinerDest cd_dst = DST_DISCARD,
                              CombinerDest sum_dst = DST_DISCARD, bool ab_dot_product = false,
                              bool cd_dot_product = false, CombinerSumMuxMode sum_or_mux = SM_SUM,
                              CombinerOutOp op = OP_IDENTITY) const;
  void ClearOutputAlphaColorCombiner(int combiner) const;
  void ClearOutputAlphaCombiners() const;

  void SetFinalCombiner0Just(CombinerSource d_source, bool d_alpha = false, bool d_invert = false) const {
    SetFinalCombiner0(SRC_ZERO, false, false, SRC_ZERO, false, false, SRC_ZERO, false, false, d_source, d_alpha,
                      d_invert);
  }
  void SetFinalCombiner0(CombinerSource a_source = SRC_ZERO, bool a_alpha = false, bool a_invert = false,
                         CombinerSource b_source = SRC_ZERO, bool b_alpha = false, bool b_invert = false,
                         CombinerSource c_source = SRC_ZERO, bool c_alpha = false, bool c_invert = false,
                         CombinerSource d_source = SRC_ZERO, bool d_alpha = false, bool d_invert = false) const;

  void SetFinalCombiner1Just(CombinerSource g_source, bool g_alpha = false, bool g_invert = false) const {
    SetFinalCombiner1(SRC_ZERO, false, false, SRC_ZERO, false, false, g_source, g_alpha, g_invert);
  }
  void SetFinalCombiner1(CombinerSource e_source = SRC_ZERO, bool e_alpha = false, bool e_invert = false,
                         CombinerSource f_source = SRC_ZERO, bool f_alpha = false, bool f_invert = false,
                         CombinerSource g_source = SRC_ZERO, bool g_alpha = false, bool g_invert = false,
                         bool specular_add_invert_r0 = false, bool specular_add_invert_v1 = false,
                         bool specular_clamp = false) const;

  void SetCombinerFactorC0(int combiner, uint32_t value) const;
  void SetCombinerFactorC0(int combiner, float red, float green, float blue, float alpha) const;
  void SetCombinerFactorC1(int combiner, uint32_t value) const;
  void SetCombinerFactorC1(int combiner, float red, float green, float blue, float alpha) const;

  void SetFinalCombinerFactorC0(uint32_t value) const;
  void SetFinalCombinerFactorC0(float red, float green, float blue, float alpha) const;
  void SetFinalCombinerFactorC1(uint32_t value) const;
  void SetFinalCombinerFactorC1(float red, float green, float blue, float alpha) const;

  static std::string PrepareSaveFile(std::string output_directory, const std::string &filename,
                                     const std::string &ext = ".png");
  uint32_t MakeInputCombiner(CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping, CombinerSource b_source,
                             bool b_alpha, CombinerMapping b_mapping, CombinerSource c_source, bool c_alpha,
                             CombinerMapping c_mapping, CombinerSource d_source, bool d_alpha,
                             CombinerMapping d_mapping) const;
  uint32_t MakeOutputCombiner(CombinerDest ab_dst, CombinerDest cd_dst, CombinerDest sum_dst, bool ab_dot_product,
                              bool cd_dot_product, CombinerSumMuxMode sum_or_mux, CombinerOutOp op) const;

 private:
  std::shared_ptr<VertexShaderProgram> vertex_shader_program_{};
  bool save_results_{true};

  uint8_t *compute_buffer_{nullptr};
  uint32_t *shader_code_{nullptr};
  uint32_t shader_code_size_{0};

  float *vertex_buffer_{nullptr};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_HOST_H
