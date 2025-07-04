/*
 * This sample provides a very basic demonstration of 3D rendering on the Xbox,
 * using pbkit. Based on the pbkit demo sources.
 */

#include <SDL_image.h>
#include <hal/debug.h>
#include <hal/fileio.h>
#include <hal/video.h>
#include <nxdk/mount.h>
#include <pbkit/pbkit.h>
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "SDL_test_fuzzer.h"
#include "debug_output.h"
#include "logger.h"
#include "pbkit_sdl_gpu.h"
#include "pushbuffer.h"
#include "test_driver.h"
#include "test_host.h"
#include "tests/americasarmyshader.h"
#include "tests/cpu_shader_tests.h"
#include "tests/exceptional_float_tests.h"
#include "tests/ilu_rcp_tests.h"
#include "tests/mac_add_tests.h"
#include "tests/mac_mov_tests.h"
#include "tests/paired_ilu_tests.h"
#include "tests/spyvsspymenu.h"
#include "tests/vertex_data_array_format_tests.h"
#include "text_overlay.h"

#ifndef FALLBACK_OUTPUT_ROOT_PATH
#define FALLBACK_OUTPUT_ROOT_PATH "e:\\";
#endif

static constexpr uint32_t kTextInsetX = 10;
static constexpr uint32_t kTextInsetY = 20;

static constexpr const char* kLogFileName = "log.txt";

static void register_suites(TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites,
                            const std::string& output_directory);
static bool get_writable_output_directory(std::string& xbe_root_directory);
static bool get_test_output_path(std::string& test_output_directory);
static void dump_config_file(const std::string& config_file_path,
                             const std::vector<std::shared_ptr<TestSuite>>& test_suites);
static void process_config(const char* config_file_path, std::vector<std::shared_ptr<TestSuite>>& test_suites);

extern "C" __cdecl int automount_d_drive(void);

/* Main program function */
int main() {
  automount_d_drive();
  XVideoSetMode(kFramebufferWidth, kFramebufferHeight, 32, REFRESH_DEFAULT);

  PBKitSDLGPUInit();
  GPU_Target* gpu_target = nullptr;
  {
    SDL_Window* window = SDL_CreateWindow("nxdk_vsh_tests", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          kFramebufferWidth, kFramebufferHeight, SDL_WINDOW_SHOWN);
    GPU_SetInitWindow(SDL_GetWindowID(window));
    gpu_target = GPU_Init(kFramebufferWidth, kFramebufferHeight, GPU_DEFAULT_INIT_FLAGS);
    if (!gpu_target) {
      debugPrint("Failed to initialize SDL_GPU.");
      debugPrint("%s", SDL_GetError());
      pb_show_debug_screen();
      Sleep(2000);
      return 1;
    }
    GPU_SetShapeBlendMode(GPU_BLEND_NORMAL);
    GPU_ClearColor(gpu_target, GPU_MakeColor(0, 0, 0, 0xFF));
  }

  debugPrint("Initializing...");
  pb_show_debug_screen();

  if (SDL_Init(SDL_INIT_GAMECONTROLLER)) {
    debugPrint("Failed to initialize SDL_GAMECONTROLLER.");
    debugPrint("%s", SDL_GetError());
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    debugPrint("Failed to initialize SDL_image PNG mode.");
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  }

  std::string test_output_directory;
  if (!get_test_output_path(test_output_directory)) {
    debugPrint("Failed to mount %s", test_output_directory.c_str());
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  };

  pb_show_front_screen();
  debugClearScreen();

  TextOverlay::Create(gpu_target, "D:\\IBMPlexMono-SemiBold.ttf", kFramebufferWidth - 2 * kTextInsetX,
                      kFramebufferHeight - 2 * kTextInsetY, kTextInsetX, kTextInsetY);
  GPU_Flip(gpu_target);

  {
    auto now = std::chrono::high_resolution_clock::now();
    auto seed = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    SDLTest_FuzzerInit(seed);
  }
  TestHost host;

  std::vector<std::shared_ptr<TestSuite>> test_suites;
  register_suites(host, test_suites, test_output_directory);

  TestHost::EnsureFolderExists(test_output_directory);

#ifdef ENABLE_PROGRESS_LOG
  {
    std::string log_file = test_output_directory + "\\" + kLogFileName;
    DeleteFile(log_file.c_str());

    Logger::Initialize(log_file, true);
  }
#endif

#ifdef DUMP_CONFIG_FILE
  dump_config_file(test_output_directory + "\\config.cnf", test_suites);
#endif

#ifdef RUNTIME_CONFIG_PATH
  process_config(RUNTIME_CONFIG_PATH, test_suites);
#endif

  Pushbuffer::Initialize();

  TestDriver driver(host, test_suites, kFramebufferWidth, kFramebufferHeight);
  driver.Run();

#ifdef ENABLE_SHUTDOWN
  HalInitiateShutdown();
#else
  debugClearScreen();
  debugPrint("Results written to %s\n\nRebooting in 4 seconds...\n", test_output_directory.c_str());
#endif
  pb_show_debug_screen();
  Sleep(4000);

  pb_kill();
  return 0;
}

static bool ensure_drive_mounted(char drive_letter) {
  if (nxIsDriveMounted(drive_letter)) {
    return true;
  }

  char dos_path[4] = "x:\\";
  dos_path[0] = drive_letter;
  char device_path[256] = {0};
  if (XConvertDOSFilenameToXBOX(dos_path, device_path) != STATUS_SUCCESS) {
    return false;
  }

  if (!strstr(device_path, R"(\Device\Harddisk0\Partition)")) {
    return false;
  }
  device_path[28] = 0;

  return nxMountDrive(drive_letter, device_path);
}

static bool get_writable_output_directory(std::string& xbe_root_directory) {
  std::string xbe_directory = XeImageFileName->Buffer;
  if (xbe_directory.find("\\Device\\CdRom") == 0) {
    debugPrint("Running from readonly media, using default path for test output.\n");
    xbe_root_directory = FALLBACK_OUTPUT_ROOT_PATH;

    std::replace(xbe_root_directory.begin(), xbe_root_directory.end(), '/', '\\');

    return ensure_drive_mounted(xbe_root_directory.front());
  }

  xbe_root_directory = "D:";
  return true;
}

static bool get_test_output_path(std::string& test_output_directory) {
  if (!get_writable_output_directory(test_output_directory)) {
    return false;
  }
  char last_char = test_output_directory.back();
  if (last_char == '\\' || last_char == '/') {
    test_output_directory.pop_back();
  }
  test_output_directory += "\\nxdk_vsh_tests";
  return true;
}

static void dump_config_file(const std::string& config_file_path,
                             const std::vector<std::shared_ptr<TestSuite>>& test_suites) {
  if (!ensure_drive_mounted(config_file_path[0])) {
    ASSERT(!"Failed to mount config path")
  }

  const char* out_path = config_file_path.c_str();
  PrintMsg("Writing config file to %s\n", out_path);

  std::ofstream config_file(config_file_path);
  ASSERT(config_file && "Failed to open config file for output");

  config_file << "# vsh test suite configuration" << std::endl;
  config_file << "# Lines starting with '#' are ignored." << std::endl;
  config_file << "# To enable a test suite, add its name on a single line with no leading #. E.g.," << std::endl;
  config_file << "# Lighting normals" << std::endl;
  config_file << "# To disable a single test within a suite, add the name of the test prefixed with" << std::endl;
  config_file << "#  a '-' after the uncommented suite. E.g.," << std::endl;
  config_file << "# -NoNormal" << std::endl;
  config_file << std::endl;

  for (auto& suite : test_suites) {
    config_file << suite->Name() << std::endl;
    for (auto& test_name : suite->TestNames()) {
      config_file << "# " << test_name << std::endl;
    }
    config_file << std::endl << "#-------------------" << std::endl;
  }
}

static void process_config(const char* config_file_path, std::vector<std::shared_ptr<TestSuite>>& test_suites) {
  if (!ensure_drive_mounted(config_file_path[0])) {
    ASSERT(!"Failed to mount config path")
  }

  std::string dos_style_path = config_file_path;
  std::replace(dos_style_path.begin(), dos_style_path.end(), '/', '\\');
  std::map<std::string, std::vector<std::string>> test_config;
  std::ifstream config_file(dos_style_path.c_str());
  ASSERT(config_file && "Failed to open config file");

  // The config file is a list of test suite names (one per line), each optionally followed by lines containing a test
  // name prefixed with '-' (indicating that test should be disabled).
  std::string last_test_suite;
  std::string line;
  while (std::getline(config_file, line)) {
    if (line.empty()) {
      continue;
    }
    if (line.front() == '-') {
      line.erase(0, 1);
      test_config[last_test_suite].push_back(line);
      continue;
    }
    if (line.front() == '#') {
      continue;
    }

    test_config[line] = {};
    last_test_suite = line;
  }

  std::vector<std::shared_ptr<TestSuite>> filtered_tests;
  for (auto& suite : test_suites) {
    auto config = test_config.find(suite->Name());
    if (config == test_config.end()) {
      continue;
    }

    if (!config->second.empty()) {
      suite->DisableTests(config->second);
    }
    filtered_tests.push_back(suite);
  }

  if (filtered_tests.empty()) {
    return;
  }
  test_suites = filtered_tests;
}

static void register_suites(TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites,
                            const std::string& output_directory) {

#define REG_TEST(CLASS_NAME)                                                   \
  {                                                                            \
    auto suite = std::make_shared<CLASS_NAME>(host, output_directory); \
    test_suites.push_back(suite);                                              \
  }

  // -- Begin REG_TEST --

  REG_TEST(Americasarmyshader)
  REG_TEST(CpuShaderTests)
  REG_TEST(ExceptionalFloatTests)
  REG_TEST(IluRcpTests)
  REG_TEST(MACMovTests)
  REG_TEST(MacAddTests)
  REG_TEST(PairedIluTests)
  REG_TEST(Spyvsspymenu)
  REG_TEST(VertexDataArrayFormatTests)

    // -- End REG_TEST --

  #undef REG_TEST
}
