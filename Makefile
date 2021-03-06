XBE_TITLE = nxdk_vsh_tests
GEN_XISO = $(XBE_TITLE).iso
NXDK_DIR ?= $(CURDIR)/third_party/nxdk
NXDK_SDL = y
NXDK_CXX = y

# pip3 install nv2a-vsh
# https://pypi.org/project/nv2a-vsh/
# https://github.com/abaire/nv2a_vsh_asm
NV2AVSH = nv2avsh

RESOURCEDIR = $(CURDIR)/resources
SRCDIR = $(CURDIR)/src
THIRDPARTYDIR = $(CURDIR)/third_party

OPTIMIZED_SRCS = \
	$(SRCDIR)/debug_output.cpp \
	$(SRCDIR)/logger.cpp \
	$(SRCDIR)/math3d.c \
	$(SRCDIR)/pbkit_ext.cpp \
	$(SRCDIR)/pgraph_diff_token.cpp \
	$(SRCDIR)/test_driver.cpp \
	$(THIRDPARTYDIR)/compareasint/compare_as_int.cpp \
    $(THIRDPARTYDIR)/fpng/src/fpng.cpp \
	$(THIRDPARTYDIR)/nv2a_vsh_cpu/src/nv2a_vsh_cpu.c \
	$(THIRDPARTYDIR)/nxdk/lib/sdl/SDL2/src/test/SDL_test_fuzzer.c \
	$(THIRDPARTYDIR)/printf/printf.c \
	$(THIRDPARTYDIR)/SDL_FontCache/SDL_FontCache.c

SRCS = \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/menu_item.cpp \
	$(SRCDIR)/shaders/vertex_shader_program.cpp \
	$(SRCDIR)/text_overlay.cpp \
	$(SRCDIR)/test_host.cpp \
	$(SRCDIR)/tests/mac_mov_tests.cpp \
	$(SRCDIR)/tests/test_suite.cpp \
	$(SRCDIR)/tests/mac_add_tests.cpp \
	$(SRCDIR)/tests/americasarmyshader.cpp \
	$(SRCDIR)/tests/ilu_rcp_tests.cpp \
	$(SRCDIR)/tests/paired_ilu_tests.cpp \
	$(SRCDIR)/tests/vertex_data_array_format_tests.cpp \
	$(SRCDIR)/tests/spyvsspymenu.cpp \
	$(SRCDIR)/tests/cpu_shader_tests.cpp

NV2A_VSH_OBJS = \
	$(SRCDIR)/shaders/clear_state.vshinc \
	$(SRCDIR)/shaders/compute_footer.vshinc \
	$(SRCDIR)/shaders/ilu_exp_passthrough.vshinc \
	$(SRCDIR)/shaders/ilu_lit_passthrough.vshinc \
	$(SRCDIR)/shaders/ilu_log_passthrough.vshinc \
	$(SRCDIR)/shaders/ilu_rcc_passthrough.vshinc \
	$(SRCDIR)/shaders/ilu_rcp.vshinc \
	$(SRCDIR)/shaders/ilu_rcp_passthrough.vshinc \
	$(SRCDIR)/shaders/ilu_rsq_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_add.vshinc \
	$(SRCDIR)/shaders/mac_add_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_arl_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_dp3_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_dp4_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_dph_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_dst_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_mad_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_max_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_min_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_mov.vshinc \
	$(SRCDIR)/shaders/mac_mov_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_mul_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_sge_passthrough.vshinc \
	$(SRCDIR)/shaders/mac_slt_passthrough.vshinc \
	$(SRCDIR)/shaders/americas_army_shader.vshinc \
	$(SRCDIR)/shaders/paired_ilu_non_r1_temp_out.vshinc \
	$(SRCDIR)/shaders/vertex_data_array_format_passthrough.vshinc \
	$(SRCDIR)/shaders/spyvsspymenu.vshinc

INCLUDE_PATHS = \
	-I$(SRCDIR) \
	-I$(THIRDPARTYDIR) \
	-I$(THIRDPARTYDIR)/nv2a_vsh_cpu/src \
	-I$(THIRDPARTYDIR)/sdl-gpu/include

CFLAGS += $(INCLUDE_PATHS) -DFC_USE_SDL_GPU
CXXFLAGS += $(INCLUDE_PATHS) -DFPNG_NO_STDIO=1 -DFPNG_NO_SSE=1 -DFC_USE_SDL_GPU

override SDL_GPU_DIR := $(THIRDPARTYDIR)/sdl-gpu
include $(THIRDPARTYDIR)/pbkit-sdl-gpu/Makefile.inc

OPTIMIZE_COMPILE_FLAGS = -O3 -fno-strict-aliasing
ifneq ($(DEBUG),y)
CFLAGS += $(OPTIMIZE_COMPILE_FLAGS)
CXXFLAGS += $(OPTIMIZE_COMPILE_FLAGS)
endif

# Disable automatic test execution if no input is detected.
DISABLE_AUTORUN ?= n
# Remove the delay for input before starting automated testing.
AUTORUN_IMMEDIATELY ?= n
ifeq ($(DISABLE_AUTORUN),y)
CXXFLAGS += -DDISABLE_AUTORUN
else
ifeq ($(AUTORUN_IMMEDIATELY),y)
CXXFLAGS += -DAUTORUN_IMMEDIATELY
endif
endif

# Cause the program to shut down the xbox on completion instead of rebooting.
ENABLE_SHUTDOWN ?= n
ifeq ($(ENABLE_SHUTDOWN),y)
CXXFLAGS += -DENABLE_SHUTDOWN
endif

# Optionally set the root path at which test results will be written when running
# from read-only media.
# E.g., "c:"
ifdef FALLBACK_OUTPUT_ROOT_PATH
CXXFLAGS += -DFALLBACK_OUTPUT_ROOT_PATH="\"$(FALLBACK_OUTPUT_ROOT_PATH)\""
endif

# Set the path to a configuration file containing the names of tests that should
# be enabled, one per line.
# E.g., "c:/vsh_tests.cnf"
ifdef RUNTIME_CONFIG_PATH
CXXFLAGS += -DRUNTIME_CONFIG_PATH="\"$(RUNTIME_CONFIG_PATH)\""
endif

# Cause a runtime config file enabling all tests to be generated in the standard results directory.
DUMP_CONFIG_FILE ?= n
ifeq ($(DUMP_CONFIG_FILE),y)
CXXFLAGS += -DDUMP_CONFIG_FILE
endif

# Cause a log file to be created and updated with metrics at the start and end of each test.
ENABLE_PROGRESS_LOG ?= n
ifeq ($(ENABLE_PROGRESS_LOG),y)
CXXFLAGS += -DENABLE_PROGRESS_LOG
endif

CLEANRULES = clean-optimized clean-nv2a-vsh-objs clean-resources
include $(NXDK_DIR)/Makefile

PBKIT_DEBUG ?= n
ifeq ($(PBKIT_DEBUG),y)
NXDK_CFLAGS += -DDBG
endif

XBDM_GDB_BRIDGE := xbdm
REMOTE_PATH := e:\\vsh
XBOX ?=
.phony: deploy
deploy: $(OUTPUT_DIR)/default.xbe
	$(XBDM_GDB_BRIDGE) $(XBOX) -- mkdir $(REMOTE_PATH)
	# TODO: Support moving the actual changed files.
	# This hack will only work if the default.xbe changes when any resource changes.
	$(XBDM_GDB_BRIDGE) $(XBOX) -- putfile $(OUTPUT_DIR)/ $(REMOTE_PATH) -f

.phony: execute
execute: deploy
	$(XBDM_GDB_BRIDGE) $(XBOX) -s -- /run $(REMOTE_PATH)

.phony: debug_bridge_no_deploy
debug_bridge_no_deploy:
	$(XBDM_GDB_BRIDGE) $(XBOX) -s -- gdb :1999 '&&' /launch $(REMOTE_PATH)

.phony: debug_bridge
debug_bridge: deploy debug_bridge_no_deploy

DEPS += $(filter %.c.d, $(OPTIMIZED_SRCS:.c=.c.d))
DEPS += $(filter %.cpp.d, $(OPTIMIZED_SRCS:.cpp=.cpp.d))
$(OPTIMIZED_SRCS): $(SHADER_OBJS) $(NV2A_VSH_OBJS)
OPTIMIZED_CC_SRCS := $(filter %.c,$(OPTIMIZED_SRCS))
OPTIMIZED_CC_OBJS := $(addsuffix .obj, $(basename $(OPTIMIZED_CC_SRCS)))
OPTIMIZED_CXX_SRCS := $(filter %.cpp,$(OPTIMIZED_SRCS))
OPTIMIZED_CXX_OBJS := $(addsuffix .obj, $(basename $(OPTIMIZED_CXX_SRCS)))
OPTIMIZED_AS_SRCS := $(filter %.s,$(OPTIMIZED_SRCS))
OPTIMIZED_AS_OBJS := $(addsuffix .obj, $(basename $(OPTIMIZED_AS_SRCS)))
OPTIMIZED_OBJS := $(OPTIMIZED_CC_OBJS) $(OPTIMIZED_CXX_OBJS) $(OPTIMIZED_AS_OBJS)
OPTIMIZED_FILTER_COMPILE_FLAGS := -g -gdwarf-4

$(OPTIMIZED_CC_OBJS): %.obj: %.c
	@echo "[ CC - OPT ] $@"
	$(VE) $(CC) $(filter-out $(OPTIMIZED_FILTER_COMPILE_FLAGS),$(NXDK_CFLAGS)) $(OPTIMIZE_COMPILE_FLAGS) $(CFLAGS) -MD -MP -MT '$@' -MF '$(patsubst %.obj,%.c.d,$@)' -c -o '$@' '$<'

$(OPTIMIZED_CXX_OBJS): %.obj: %.cpp
	@echo "[ CXX - OPT] $@"
	$(VE) $(CXX) $(filter-out $(OPTIMIZED_FILTER_COMPILE_FLAGS),$(NXDK_CXXFLAGS)) $(OPTIMIZE_COMPILE_FLAGS) $(CXXFLAGS) -MD -MP -MT '$@' -MF '$(patsubst %.obj,%.cpp.d,$@)' -c -o '$@' '$<'

$(OPTIMIZED_AS_OBJS): %.obj: %.s
	@echo "[ AS - OPT ] $@"
	$(VE) $(AS) $(filter-out $(OPTIMIZED_FILTER_COMPILE_FLAGS),$(NXDK_ASFLAGS)) $(OPTIMIZE_COMPILE_FLAGS) $(ASFLAGS) -c -o '$@' '$<'

optimized.lib: $(OPTIMIZED_OBJS)

.PHONY: clean-optimized
clean-optimized:
	$(VE)rm -f optimized.lib $(OPTIMIZED_OBJS)

main.exe: optimized.lib

# nv2avsh assembler rules:
$(SRCS): $(NV2A_VSH_OBJS)
.PHONY: clean-nv2a-vsh-objs
clean-nv2a-vsh-objs:
	$(VE)rm -f $(NV2A_VSH_OBJS)

$(NV2A_VSH_OBJS): %.vshinc: %.vsh
	@echo "[ nv2avsh  ] $@"
	$(VE) $(NV2AVSH) '$<' '$@'

# Resources
RESOURCE_FILES = $(shell find $(RESOURCEDIR)/ -type f)
RESOURCES = \
	$(patsubst $(RESOURCEDIR)/%,$(OUTPUT_DIR)/%,$(RESOURCE_FILES))

TARGET += $(RESOURCES)
$(GEN_XISO): $(RESOURCES)

$(OUTPUT_DIR)/%: $(RESOURCEDIR)/%
	$(VE)mkdir -p '$(dir $@)'
	$(VE)cp -r '$<' '$@'

.PHONY: clean-resources
clean-resources:
	$(VE)rm -rf $(patsubst $(RESOURCEDIR)/%,$(OUTPUT_DIR)/%,$(RESOURCES))
