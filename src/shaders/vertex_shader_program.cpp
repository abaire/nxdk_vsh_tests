#include "vertex_shader_program.h"

#include <pbkit/pbkit.h>

#include <memory>

#include "debug_output.h"
#include "pbkit_ext.h"

void VertexShaderProgram::LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const {
  uint32_t *p;
  int i;

  p = pb_begin();

  // Set run address of shader
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);

  p = pb_push1(
      p, NV097_SET_TRANSFORM_EXECUTION_MODE,
      MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
          MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));

  // Enable writing to c0-96 registers?
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, true);
  pb_end(p);

  // Set cursor and begin copying program
  p = pb_begin();
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
  pb_end(p);

  for (i = 0; i < shader_size / 16; i++) {
    p = pb_begin();
    pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
    memcpy(p, &shader[i * 4], 4 * 4);
    p += 4;
    pb_end(p);
  }
}

void VertexShaderProgram::Activate() {
  OnActivate();

  if (shader_override_) {
    LoadShaderProgram(shader_override_, shader_override_size_);
  } else {
    OnLoadShader();
  }
}

void VertexShaderProgram::PrepareDraw() {
  OnLoadConstants();

  if (!uniform_upload_required_) {
    return;
  }

  UploadConstants();
}

void VertexShaderProgram::UploadConstants() {
  uint32_t load_index = 0xFFFF;

  auto p = pb_begin();
  uint32_t depth = 0;

  for (const auto &item : uniforms_) {
    const uint32_t slot = item.first;

    if (slot != load_index) {
      p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, slot);
      load_index = slot;
    }
    p = pb_push4(p, NV097_SET_TRANSFORM_CONSTANT, item.second.x, item.second.y, item.second.z, item.second.w);
    load_index += 1;

    if (++depth > 16) {
      pb_end(p);
      while (pb_busy())
        ;
      depth = 0;
      p = pb_begin();
    }
  }

  pb_end(p);
  uniform_upload_required_ = false;
}

void VertexShaderProgram::SetUniform4x4F(uint32_t slot, const float *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void VertexShaderProgram::SetUniform4F(uint32_t slot, const float *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void VertexShaderProgram::SetUniform4UI(uint32_t slot, const uint32_t *value) { SetUniformBlock(slot, value, 1); }

void VertexShaderProgram::SetUniformUI(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  const uint32_t vector[] = {x, y, z, w};
  SetUniform4UI(slot, vector);
}

void VertexShaderProgram::SetUniform4I(uint32_t slot, const int32_t *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void VertexShaderProgram::SetUniformI(uint32_t slot, int32_t x, int32_t y, int32_t z, int32_t w) {
  const int32_t vector[] = {x, y, z, w};
  SetUniform4I(slot, vector);
}

void VertexShaderProgram::SetUniformF(uint32_t slot, float x, float y, float z, float w) {
  const float vector[] = {x, y, z, w};
  SetUniform4F(slot, vector);
}

void VertexShaderProgram::SetUniformBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots) {
  for (auto i = 0; i < num_slots; ++i, ++slot) {
    TransformConstant val = {*values++, *values++, *values++, *values++};
    uniforms_[slot] = val;
  }
  uniform_upload_required_ = true;
}
