nxdk_vsh_tests
====

Test suite for the original Xbox nv2a vertex shader.

Golden test results from XBOX hardware are [available here](https://github.com/abaire/nxdk_vsh_tests_golden_results)

## Usage

Tests will be executed automatically if no gamepad input is given within an initial timeout.

Individual tests may be executed via the menu.

### Test configuration
Tests can optionally be determined via a configuration file loaded from the XBOX. The file follows a simple line-based
format.

* A test suite and all of the tests within will be enabled if the suite name appears on a single line.
* A single test within an enabled suite may be disabled by prefixing its name with a `-` after the line containing the
  test suite containing it.
* Any line starting with `#` will be ignored.

The names used are the same as the names that appear in the `nxdk_vsh_tests` menu.

If the entire file is empty (or commented out), it will be ignored and all tests will be enabled.

For example:
```
ThisTestSuiteIsEnabled
-ExceptThisTest

# This is ignored.
```

### Controls

DPAD:

* Up - Move the menu cursor up. Inside of a test, go to the previous test in the active suite.
* Down - Move the menu cursor down. Inside of a test, go to the previous test in the active suite.
* Left - Move the menu cursor up by half a page.
* Right - Move the menu cursor down by half a page.
* A - Enter a submenu or test. Inside of a test, re-run the test.
* B - Go up one menu or leave a test. If pressed on the root menu, exit the application.
* X - Run all tests for the current suite.
* Y - Toggle saving of results and multi-frame rendering.
* Start - Enter a submenu or test.
* Back - Go up one menu or leave a test. If pressed on the root menu, exit the application.
* Black - Exit the application.

## Build prerequisites

This project uses [nv2a-vsh](https://pypi.org/project/nv2a-vsh/) to assemble some of the vertex shaders for tests.
It can be installed via `pip3 install nv2a-vsh --upgrade`.

This test suite requires some modifications to the pbkit used by the nxdk in order to operate.

To facilitate this, the nxdk is included as a submodule of this project, referencing the
`pbkit_extensions` branch from https://github.com/abaire/nxdk.

This project should be cloned with the `--recursive` flag to pull the submodules and their submodules,
after the fact this can be achieved via `git submodule update --init --recursive`.

As of June 2025, the nxdk's CMake implementation requires bootstrapping before it may be used. To facilitate this, run
the `prewarm-nxdk.sh` script from this project's root directory. It will navigate into the `nxdk` subdir and build all
the sample projects, triggering the creation of the `nxdk` libraries needed for the toolchain and for this project.

## Running with CLion

Create a build target

1. Create a new `Embedded GDB Server` target
1. Set the Target to `all`
1. Set the Executable to `main.exe`
1. Set `Download executable` to `None`
1. Set `'target remote' args` to `127.0.0.1:1234`
1. Set `GDB Server` to the path to the xemu binary
1. Set `GDB Server args` to `-s -S` (the `-S` is optional and will cause xemu to wait for the debugger to connnect)

To capture DbgPrint, additionally append `-device lpc47m157 -serial tcp:127.0.0.1:9091` to `GDB Server args` and use
something like [pykdclient](https://github.com/abaire/pykdclient).

## Deploying with xbdm_gdb_bridge

The `Makefile` contains a `deploy` target that will copy the finished binary to an XBOX running XBDM. This functionality
requires the [xbdm_gdb_bridge](https://github.com/abaire/xbdm_gdb_bridge) utility.
