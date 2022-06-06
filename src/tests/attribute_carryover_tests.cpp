#include "attribute_carryover_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"

//// clang format off
//static constexpr uint32_t kShader[] = {
//#include "shaders/attribute_carryover_test.inl"
//};
//// clang format on

AttributeCarryoverTests::AttributeCarryoverTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib carryover") {
//  for (auto primitive : kPrimitives) {
//    for (auto attr : kTestAttributes) {
//      for (auto config : kTestConfigs) {
//        std::string name = MakeTestName(primitive, attr, config);
//        tests_[name] = [this, primitive, attr, config]() { this->Test(primitive, attr, config); };
//      }
//    }
//  }
}

void AttributeCarryoverTests::Initialize() {
  TestSuite::Initialize();

//  auto shader = std::make_shared<PrecalculatedVertexShader>();
//  shader->SetShaderOverride(kShader, sizeof(kShader));
//
//  // Send shader constants (see attribute_carryover_test.inl)
//  // const c[2] = 1 0 2 3
//  shader->SetUniformF(1, 1, 0, 2, 3);
//  // const c[3] = 8 9 10 11
//  shader->SetUniformF(2, 8, 9, 10, 11);
//  // const c[4] = 1 0 0.75
//  shader->SetUniformF(3, 1.0f, 0.0f, 0.75f);
//
//  host_.SetVertexShaderProgram(shader);
}

void AttributeCarryoverTests::Test() {
//  auto shader = host_.GetShaderProgram();
//
//  static constexpr uint32_t kBackgroundColor = 0xFF444444;
//  CreateGeometry(primitive);
//  host_.PrepareDraw(kBackgroundColor);
//
//  auto draw = [this, config, primitive](uint32_t additional_fields) {
//    uint32_t attributes = host_.POSITION | additional_fields;
//    switch (config.draw_mode) {
//      case DRAW_ARRAYS:
//        host_.DrawArrays(attributes, primitive);
//        break;
//
//      case DRAW_INLINE_BUFFERS:
//        host_.DrawInlineBuffer(attributes, primitive);
//        break;
//
//      case DRAW_INLINE_ELEMENTS:
//        host_.DrawInlineElements16(index_buffer_, attributes, primitive);
//        break;
//
//      case DRAW_INLINE_ARRAYS:
//        host_.DrawInlineArray(attributes, primitive);
//        break;
//    }
//  };
//
//  // TODO: Set the attribute under test.
//  auto vertex = bleed_buffer_->Lock();
//  switch (primitive) {
//    case TestHost::PRIMITIVE_LINES:
//      // Set the second vertex in the line.
//      ++vertex;
//      break;
//
//    case TestHost::PRIMITIVE_TRIANGLES:
//      // Set the last vertex in the triangle.
//      vertex += 2;
//      break;
//
//    default:
//      ASSERT(!"TODO: Implement additional primitives.");
//      break;
//  }
//
//  switch (test_attribute) {
//    case ATTR_WEIGHT:
//      vertex->SetWeight(config.attribute_value);
//      break;
//    case ATTR_NORMAL:
//      vertex->SetNormal(config.attribute_value);
//      break;
//    case ATTR_DIFFUSE:
//      vertex->SetDiffuse(config.attribute_value);
//      break;
//    case ATTR_SPECULAR:
//      vertex->SetSpecular(config.attribute_value);
//      break;
//    case ATTR_FOG_COORD:
//    case ATTR_POINT_SIZE:
//    case ATTR_BACK_DIFFUSE:
//    case ATTR_BACK_SPECULAR:
//      ASSERT(!"Attribute not supported.");
//      break;
//    case ATTR_TEX0:
//      vertex->SetTexCoord0(config.attribute_value);
//      break;
//    case ATTR_TEX1:
//      vertex->SetTexCoord1(config.attribute_value);
//      break;
//    case ATTR_TEX2:
//      vertex->SetTexCoord2(config.attribute_value);
//      break;
//    case ATTR_TEX3:
//      vertex->SetTexCoord3(config.attribute_value);
//      break;
//  }
//  bleed_buffer_->Unlock();
//
//  // The bleed buffer should always render its diffuse color, regardless of the attribute under test.
//  shader->SetUniformF(0, ATTR_DIFFUSE);
//  host_.SetVertexBuffer(bleed_buffer_);
//  draw(host_.DIFFUSE | TestAttributeToVertexAttribute(test_attribute));
//
//  // The test buffer should render the attribute under test as its diffuse color.
//  shader->SetUniformF(0, test_attribute);
//  host_.SetVertexBuffer(test_buffer_);
//  draw(0);
//
//  std::string name = MakeTestName(primitive, test_attribute, config);
//  pb_print("%s", name.c_str());
//  pb_draw_text_screen();
//
//  host_.FinishDraw(allow_saving_, output_dir_, name);
}
