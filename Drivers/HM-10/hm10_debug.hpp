/*
 * hm10_debug.hpp
 *
 *  Created on: Mar 5, 2021
 *      Author: SteelPh0enix
 */

#pragma once

#define HM10_DEBUG
//#define HM10_DEBUG_LOWLEVEL

#ifdef HM10_DEBUG
#include "printf.h"
#define debugLog(format, ...) printf("[HM-10] <%s:%d>: " format "\n", __func__, __LINE__, ## __VA_ARGS__)
#else
#define debugLog(...)
#endif

#ifdef HM10_DEBUG_LOWLEVEL
#define debugLogLL(format, ...) printf("[HM-10][LL] <%s:%d>: " format "\n", __func__, __LINE__, ## __VA_ARGS__)
#else
#define debugLogLL(...)
#endif
