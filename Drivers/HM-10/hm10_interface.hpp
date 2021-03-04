/*
 * hm10_interface.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#pragma once
#include "hm10_constants.hpp"

#include <cstdint>

namespace HM10 {

class Interface {
public:
  // Virtual d-tor for correct deconstruction
  virtual ~Interface() = default;

  // Transmit function - implement it with data transmission code.
  // If this function is non-blocking (for example, you're using DMA or interrupts), you also have to
  // re-implement `transmitComplete` with code that checks for completion (returns `false` until it's done).
  // If this function is blocking, you can ignore `transmitComplete`.
  virtual int transmit(char* data, std::size_t amount) = 0;
  virtual bool transmitComplete() {
    return true;
  }

  // Receive function - implement it with data retrieval code.
  // If this function is non-blocking (for example, you're using DMA or interrupts), you also have to
  // re-implement `receiveComplete` with code that checks for completion (returns `false` until it's done).
  // If this function is blocking, you can ignore `receiveComplete`.
  virtual int receive(char* data_buffer, std::size_t buffer_size) = 0;
  virtual bool receiveComplete() {
    return true;
  }

  // Implement this function with code that changes the baud rate of your UART peripheral.
  // Otherwise, you won't be able to change the baud rate of HM-10.
  virtual int setBaudrate(Baudrate new_baudrate) = 0;
};

}
