/*
 * hm10_constants.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#pragma once
#include <cstdint>

namespace HM10 {

enum class Baudrate : std::uint8_t {
  Baud1200 = 7,
  Baud2400 = 6,
  Baud4800 = 5,
  Baud9600 = 0,
  Baud19200 = 1,
  Baud38400 = 2,
  Baud57600 = 3,
  Baud115200 = 4,
  Baud230400 = 8
};

}
