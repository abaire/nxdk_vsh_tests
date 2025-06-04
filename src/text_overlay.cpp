#include "text_overlay.h"

#include "debug_output.h"
#include "precalculated_vertex_shader.h"
#include "pbkit/pbkit.h"

TextOverlay *TextOverlay::singleton_ = nullptr;

void TextOverlay::Create(GPU_Target *target, const char *font_path, float width, float height, float x, float y,
                         uint32_t font_size) {
  ASSERT(!singleton_);

  singleton_ = new TextOverlay(target, font_path, font_size, x, y, width, height);
}

TextOverlay::TextOverlay(GPU_Target *target, const char *font_path, uint32_t size, float x, float y, float width,
                         float height)
    : target_(target),
      ttf_file_(font_path),
      font_size_(size),
      overlay_x_(x),
      overlay_y_(y),
      overlay_width_(width),
      overlay_height_(height) {
  font_ = FC_CreateFont();
  FC_SetFilterMode(font_, FC_FILTER_NEAREST);
  auto result = FC_LoadFont(font_, font_path, size, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);
  ASSERT(result && "Failed to load font.");

  cell_size_ = FC_GetBounds(font_, 0, 0, FC_ALIGN_LEFT, FC_MakeScale(1.0f, 1.0f), "X");
}

TextOverlay::~TextOverlay() { FC_FreeFont(font_); }

void TextOverlay::Reset() {
  singleton_->cursor_x_ = 0;
  singleton_->cursor_y_ = 0;
  singleton_->content_.clear();
}

void TextOverlay::PrintAt_(uint32_t x, uint32_t y, const std::string &str) {
  auto pixel_x = static_cast<float>(overlay_x_) + static_cast<float>(x) * cell_size_.w;
  auto pixel_y = static_cast<float>(overlay_y_) + static_cast<float>(y) * cell_size_.h;

  content_.emplace_back(pixel_x, pixel_y, str);

  auto last_line_start = str.rfind('\n');
  if (last_line_start != std::string::npos) {
    auto num_lines = std::count(str.begin(), str.end(), '\n');
    x = str.size() - (last_line_start + 1);
    y += num_lines;
  } else {
    x += str.size();
  }

  if (static_cast<float>(x + 1) * cell_size_.w > overlay_width_) {
    x = 0;
    ++y;
  }

  cursor_x_ = x;
  cursor_y_ = y;
}

static void SetCombiners() {
  static constexpr uint32_t SRC_ZERO = 0;
  static constexpr uint32_t SRC_TEX0 = 8;

#define CHANNEL(src, alpha, invert) ((src) + ((alpha) << 4) + ((invert) << 5))

  static constexpr uint32_t c0 = (CHANNEL(SRC_ZERO, false, false) << 24) + (CHANNEL(SRC_ZERO, false, false) << 16) +
                                 (CHANNEL(SRC_ZERO, false, false) << 8) + CHANNEL(SRC_TEX0, false, false);
  static constexpr uint32_t c1 = (CHANNEL(SRC_ZERO, true, false) << 24) + (CHANNEL(SRC_ZERO, true, false) << 16) +
                                 (CHANNEL(SRC_TEX0, true, false) << 8);
#undef CHANNEL
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, c0);
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1, c1);
  pb_end(p);
}

void TextOverlay::Render_() const {
  SetCombiners();
  PbkitSdlGpu::LoadPrecalculatedVertexShader();
  for (auto &entry : content_) {
    FC_Draw(font_, target_, entry.x, entry.y, "%s", entry.str.c_str());
  }
}
