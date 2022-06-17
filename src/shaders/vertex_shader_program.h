#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H

#include <cstdint>
#include <map>
#include <vector>

class VertexShaderProgram {
 public:
  VertexShaderProgram() = default;

  void Activate();
  void PrepareDraw();

  // Note: It is the caller's responsibility to ensure that the shader array remains valid for the lifetime of this
  // instance.
  void SetShaderOverride(const uint32_t *shader, uint32_t shader_size) {
    shader_override_ = shader;
    shader_override_size_ = shader_size;
  }

  void SetUniform4x4F(uint32_t slot, const float *value);
  void SetUniform4F(uint32_t slot, const float *value);
  void SetUniform4UI(uint32_t slot, const uint32_t *value);
  void SetUniform4I(uint32_t slot, const int32_t *value);

  void SetUniformF(uint32_t slot, float x, float y = 0.0f, float z = 0.0f, float w = 0.0f);
  void SetUniformUI(uint32_t slot, uint32_t x, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0);
  void SetUniformI(uint32_t slot, int32_t x, int32_t y = 0, int32_t z = 0, int32_t w = 0);

 protected:
  virtual void OnActivate() {}
  virtual void OnLoadShader() {}
  virtual void OnLoadConstants(){};

  void LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const;
  void SetUniformBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots);

  void UploadConstants();

 protected:
  const uint32_t *shader_override_{nullptr};
  uint32_t shader_override_size_{0};

  struct TransformConstant {
    uint32_t x, y, z, w;
  };
  std::map<uint32_t, TransformConstant> uniforms_;
  bool uniform_upload_required_{true};
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H
