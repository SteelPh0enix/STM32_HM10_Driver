/*
 * hm10_constants.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#pragma once
#include <cstdint>

namespace HM10 {

// Enumeration with baudrates compatible with HM-10
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

// Array which can be used to get integer values of baudrate from enumeraton above
// For example, HM10::BaudrateValues[static_cast<std::uint8_t>(Baudrate::Baud115200)]
// will return 115200.
constexpr std::uint32_t BaudrateValues[] = { 9600,
                                             19200,
                                             38400,
                                             57600,
                                             115200,
                                             4800,
                                             2400,
                                             1200,
                                             230400 };

}
