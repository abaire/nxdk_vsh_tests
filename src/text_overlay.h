#ifndef NXDK_VSH_TESTS_TEXT_OVERLAY_H
#define NXDK_VSH_TESTS_TEXT_OVERLAY_H

#include <list>
#include <string>
#include <utility>

#include "SDL_gpu.h"
#include "SDL_FontCache.h"
#include "printf/printf.h"

class TextOverlay {
 public:
  struct StringEntry {
    float x;
    float y;
    std::string str;

    StringEntry(float x, float y, std::string str) : x(x), y(y), str(std::move(str)) {}
  };

 public:
  static void Create(GPU_Target *target, const char *font_path, float width, float height, float x = 0.0f,
                     float y = 0.0f, uint32_t font_size = 14);

  static void Reset();

  static void Render() { singleton_->Render_(); }

  template <typename... Args>
  static void Print(const char *fmt, Args... args) {
    singleton_->PrintAt(singleton_->cursor_x_, singleton_->cursor_y_, fmt, args...);
  }

  static void PrintAt(uint32_t x, uint32_t y, const char *fmt, ...) {
    std::string str;

    va_list arg_list, arg_list_copy;
    va_start(arg_list, fmt);

    va_copy(arg_list_copy, arg_list);
    auto length = vsnprintf_(nullptr, 0, fmt, arg_list_copy);
    va_end(arg_list_copy);

    str.resize(length);
    vsnprintf_(&str[0], length + 1, fmt, arg_list);
    va_end(arg_list);

    singleton_->PrintAt_(x, y, str);
  }

 private:
  TextOverlay(GPU_Target *target, const char *font_path, uint32_t size, float x, float y, float width, float height);
  ~TextOverlay();

  void PrintAt_(uint32_t x, uint32_t y, const std::string &str);
  void Render_() const;

 private:
  static TextOverlay *singleton_;

  GPU_Target *target_;
  std::string ttf_file_;
  uint32_t font_size_;
  FC_Font *font_;

  FC_Rect cell_size_{0};

  float overlay_x_;
  float overlay_y_;
  float overlay_width_;
  float overlay_height_;

  uint32_t cursor_x_{0};
  uint32_t cursor_y_{0};

  std::list<StringEntry> content_;
};

#endif  // NXDK_VSH_TESTS_TEXT_OVERLAY_H
