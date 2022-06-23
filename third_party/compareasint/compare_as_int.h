#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool almost_equal_ulps(float A, float B, int maxUlps);

#ifdef __cplusplus
};  // extern "C"
#endif
