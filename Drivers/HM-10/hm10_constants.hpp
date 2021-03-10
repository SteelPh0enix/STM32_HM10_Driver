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
  Baud230400 = 8,
  InvalidBaudrate = 9
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
                                             230400,
                                             0 }; // the last one is for invalid baudrate

struct MACAddress {
  char address[13];
};

enum class AdvertInterval : std::uint8_t {
  Adv100ms = 0x0,
  Adv252p5ms = 0x1,
  Adv211p25ms = 0x2,
  Adv318p75ms = 0x3,
  Adv417p5ms = 0x4,
  Adv546p25ms = 0x5,
  Adv760ms = 0x6,
  Adv852p5ms = 0x7,
  Adv1022p5ms = 0x8,
  Adv1285ms = 0x9,
  Adv2000ms = 0xA,
  Adv3000ms = 0xB,
  Adv4000ms = 0xC,
  Adv5000ms = 0xD,
  Adv6000ms = 0xE,
  Adv7000ms = 0xF,
  InvalidInterval = 0xFF
};

enum class AdvertType : std::uint8_t {
  All = 0, OnlyConnectLastDevice = 1, OnlyAdvertAndScanResponse = 2, OnlyAdvert = 3, Invalid = 0xFF
};

enum class ConnInterval : std::uint8_t {
  Interval7p5ms = 0,
  Interval10ms = 1,
  Interval15ms = 2,
  Interval20ms = 3,
  Interval25ms = 4,
  Interval30ms = 5,
  Interval35ms = 6,
  Interval40ms = 7,
  Interval45ms = 8,
  Interval4000ms = 9,
  InvalidInterval = 0xFF
};

enum class ConnSupervisionTimeout : std::uint8_t {
  Timeout100ms = 0,
  Timeout1000ms = 1,
  Timeout2000ms = 2,
  Timeout3000ms = 3,
  Timeout4000ms = 4,
  Timeout5000ms = 5,
  Timeout6000ms = 6,
  InvalidTimeout = 0xFF
};
}
