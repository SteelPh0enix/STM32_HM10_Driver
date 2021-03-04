/*
 * hm10.cpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#include "hm10.hpp"

namespace HM10 {

HM10::HM10(Interface* interface) {
  setInterface(interface);
}

void HM10::setInterface(Interface* interface) {
  m_interface = interface;
}

Interface* HM10::interface() const {
  return m_interface;
}

}
