/*
 * hm10.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#pragma once
#include "hm10_interface.hpp"
#include "hm10_constants.hpp"
#include <cstring>

namespace HM10 {

template<std::size_t TxBufferSize = 64>
class HM10 {
public:
  HM10(Interface* interface) {
    setInterface(interface);
  }

  void setInterface(Interface* interface) {
    m_interface = interface;
  }

  Interface* interface() const {
    return m_interface;
  }

  std::size_t bufferSize() const {
    return TxBufferSize;
  }

  // Send `AT` to check if the module is alive and communication is working
  bool isAlive() {
    std::strcpy(&m_txBuffer[0], Command::AT);
    // TODO: send data here ahd check answer
  }

private:
  Interface* m_interface { nullptr };
  char m_txBuffer[TxBufferSize];
};

}
