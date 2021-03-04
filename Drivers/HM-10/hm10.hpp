/*
 * hm10.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#pragma once
#include "hm10_interface.hpp"
#include "hm10_constants.hpp"

namespace HM10 {

class HM10 {
public:
  HM10(Interface* interface);

  void setInterface(Interface* interface);
  Interface* interface() const;

private:
  Interface* m_interface { nullptr };
};

}
