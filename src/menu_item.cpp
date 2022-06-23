#include "menu_item.h"

#include <pbkit/pbkit.h>

#include <chrono>
#include <utility>

#include "pbkit_ext.h"
#include "tests/test_suite.h"
#include "text_overlay.h"

#ifdef AUTORUN_IMMEDIATELY
static constexpr uint32_t kAutoTestAllTimeoutMilliseconds = 0;
#else
static constexpr uint32_t kAutoTestAllTimeoutMilliseconds = 3000;
#endif
static constexpr uint32_t kNumItemsPerPage = 20;
static constexpr uint32_t kNumItemsPerHalfPage = kNumItemsPerPage >> 1;

uint32_t MenuItem::menu_background_color_ = 0xFF3E003E;
bool MenuItemTest::one_shot_mode_ = true;

void MenuItem::PrepareDraw(uint32_t background_color) const {
  pb_wait_for_vbl();
  pb_target_back_buffer();
  pb_reset();
  pb_fill(0, 0, width, height, background_color);
  pb_erase_text_screen();
  TextOverlay::Reset();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BLEND_ENABLE, true);
  p = pb_push1(p, NV097_SET_BLEND_EQUATION, NV097_SET_BLEND_EQUATION_V_FUNC_ADD);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);

  p = pb_push1(
      p, NV097_SET_TRANSFORM_EXECUTION_MODE,
      MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_FIXED) |
          MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
  pb_end(p);
}

void MenuItem::Swap() {
  TextOverlay::Render();
  while (pb_finished()) {
  }
}

void MenuItem::Draw() {
  if (active_submenu) {
    active_submenu->Draw();
    return;
  }

  PrepareDraw(menu_background_color_);

  const char *cursor_prefix = "> ";
  const char *normal_prefix = "  ";
  const char *cursor_suffix = " <";
  const char *normal_suffix = "";

  uint32_t i = 0;
  if (cursor_position > kNumItemsPerHalfPage) {
    i = cursor_position - kNumItemsPerHalfPage;
    if (i + kNumItemsPerPage > submenu.size()) {
      if (submenu.size() < kNumItemsPerPage) {
        i = 0;
      } else {
        i = submenu.size() - kNumItemsPerPage;
      }
    }
  }

  if (i) {
    TextOverlay::Print("...\n");
  }

  uint32_t i_end = i + std::min(kNumItemsPerPage, submenu.size());

  for (; i < i_end; ++i) {
    const char *prefix = i == cursor_position ? cursor_prefix : normal_prefix;
    const char *suffix = i == cursor_position ? cursor_suffix : normal_suffix;
    TextOverlay::Print("%s%s%s\n", prefix, submenu[i]->name.c_str(), suffix);
  }

  if (i_end < submenu.size()) {
    TextOverlay::Print("...\n");
  }

  Swap();
}

void MenuItem::OnEnter() {}

void MenuItem::Activate() {
  if (active_submenu) {
    active_submenu->Activate();
    return;
  }

  auto activated_item = submenu[cursor_position];
  if (activated_item->IsEnterable()) {
    active_submenu = activated_item;
    activated_item->OnEnter();
  } else {
    activated_item->Activate();
  }
}

void MenuItem::ActivateCurrentSuite() {
  if (active_submenu) {
    active_submenu->ActivateCurrentSuite();
    return;
  }
  auto activated_item = submenu[cursor_position];
  activated_item->ActivateCurrentSuite();
}

bool MenuItem::Deactivate() {
  if (!active_submenu) {
    return false;
  }

  bool was_submenu_activated = active_submenu->Deactivate();
  if (!was_submenu_activated) {
    active_submenu.reset();
  }
  return true;
}

void MenuItem::CursorUp() {
  if (active_submenu) {
    active_submenu->CursorUp();
    return;
  }

  if (cursor_position > 0) {
    --cursor_position;
  } else {
    cursor_position = submenu.size() - 1;
  }
}

void MenuItem::CursorDown() {
  if (active_submenu) {
    active_submenu->CursorDown();
    return;
  }

  if (cursor_position < submenu.size() - 1) {
    ++cursor_position;
  } else {
    cursor_position = 0;
  }
}

void MenuItem::CursorLeft() {
  if (active_submenu) {
    active_submenu->CursorLeft();
    return;
  }

  if (cursor_position > kNumItemsPerHalfPage) {
    cursor_position -= kNumItemsPerHalfPage;
  } else {
    cursor_position = 0;
  }
}

void MenuItem::CursorRight() {
  if (active_submenu) {
    active_submenu->CursorRight();
    return;
  }

  cursor_position += kNumItemsPerHalfPage;
  if (cursor_position >= submenu.size()) {
    cursor_position = submenu.size() - 1;
  }
}

void MenuItem::CursorUpAndActivate() {
  active_submenu = nullptr;
  CursorUp();
  Activate();
}

void MenuItem::CursorDownAndActivate() {
  active_submenu = nullptr;
  CursorDown();
  Activate();
}

void MenuItem::SetBackgroundColor(uint32_t background_color) { menu_background_color_ = background_color; }

MenuItemCallable::MenuItemCallable(std::function<void()> callback, std::string name, uint32_t width, uint32_t height)
    : MenuItem(std::move(name), width, height), on_activate(std::move(callback)) {}

void MenuItemCallable::Draw() {}

void MenuItemCallable::Activate() { on_activate(); }

MenuItemTest::MenuItemTest(std::shared_ptr<TestSuite> suite, std::string name, uint32_t width, uint32_t height)
    : MenuItem(std::move(name), width, height), suite(std::move(suite)) {}

void MenuItemTest::Draw() {
  if (one_shot_mode_ && has_run_once_) {
    return;
  }

  suite->Run(name);
  suite->SetSavingAllowed(false);
  has_run_once_ = true;
}

void MenuItemTest::OnEnter() {
  // Blank the screen.
  PrepareDraw(0xFF000000);
  TextOverlay::Print("Running %s", name.c_str());
  Swap();

  suite->Initialize();
  suite->SetSavingAllowed(true);
  has_run_once_ = false;
}

bool MenuItemTest::Deactivate() {
  suite->Deinitialize();
  return MenuItem::Deactivate();
}

void MenuItemTest::CursorUp() { parent->CursorUpAndActivate(); }

void MenuItemTest::CursorDown() { parent->CursorDownAndActivate(); }

MenuItemSuite::MenuItemSuite(const std::shared_ptr<TestSuite> &suite, uint32_t width, uint32_t height)
    : MenuItem(suite->Name(), width, height), suite(suite) {
  auto tests = suite->TestNames();
  submenu.reserve(tests.size());

  for (auto &test : tests) {
    auto child = std::make_shared<MenuItemTest>(suite, test, width, height);
    child->parent = this;
    submenu.push_back(child);
  }
}

void MenuItemSuite::ActivateCurrentSuite() {
  suite->Initialize();
  suite->SetSavingAllowed(true);
  suite->RunAll();
  suite->Deinitialize();
  MenuItem::Deactivate();
}

MenuItemRoot::MenuItemRoot(const std::vector<std::shared_ptr<TestSuite>> &suites, std::function<void()> on_run_all,
                           std::function<void()> on_exit, uint32_t width, uint32_t height)
    : MenuItem("<<root>>", width, height), on_run_all(std::move(on_run_all)), on_exit(std::move(on_exit)) {
#ifndef DISABLE_AUTORUN
  submenu.push_back(std::make_shared<MenuItemCallable>(on_run_all, "Run all and exit", width, height));
#endif  // DISABLE_AUTORUN
  for (auto &suite : suites) {
    auto child = std::make_shared<MenuItemSuite>(suite, width, height);
    child->parent = this;
    submenu.push_back(child);
  }
#ifdef DISABLE_AUTORUN
  submenu.push_back(std::make_shared<MenuItemCallable>(on_run_all, "! Run all and exit", width, height));
#endif  // DISABLE_AUTORUN
  start_time = std::chrono::high_resolution_clock::now();
}

void MenuItemRoot::ActivateCurrentSuite() {
  timer_cancelled = true;
  MenuItem::ActivateCurrentSuite();
}

void MenuItemRoot::Draw() {
#ifndef DISABLE_AUTORUN
  if (!timer_cancelled) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    if (elapsed > kAutoTestAllTimeoutMilliseconds) {
      on_run_all();
      return;
    }

    char run_all[128] = {0};
    snprintf(run_all, 127, "Run all and exit (automatic in %d ms)", kAutoTestAllTimeoutMilliseconds - elapsed);
    submenu[0]->name = run_all;
  } else {
    submenu[0]->name = "Run all and exit";
  }
#endif  // DISABLE_AUTORUN
  MenuItem::Draw();
}

void MenuItemRoot::Activate() {
  timer_cancelled = true;
  MenuItem::Activate();
}

bool MenuItemRoot::Deactivate() {
  timer_cancelled = true;
  if (!active_submenu) {
    on_exit();
    return false;
  }

  return MenuItem::Deactivate();
}

void MenuItemRoot::CursorUp() {
  timer_cancelled = true;
  MenuItem::CursorUp();
}

void MenuItemRoot::CursorDown() {
  timer_cancelled = true;
  MenuItem::CursorDown();
}

void MenuItemRoot::CursorLeft() {
  timer_cancelled = true;
  MenuItem::CursorLeft();
}

void MenuItemRoot::CursorRight() {
  timer_cancelled = true;
  MenuItem::CursorRight();
}
