/*
 * hm10_interface_stm32_hal_blocking.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#pragma once
#include "../hm10_interface.hpp"
// Change this include if you're using different STM32 series
#include <stm32f4xx.h>

namespace HM10 {

class InterfaceSTM32HALBlocking : public Interface {
public:
  // Timeout is in milliseconds
  InterfaceSTM32HALBlocking(UART_HandleTypeDef* huart, std::uint32_t timeout = 500);

  virtual int transmit(char* data, std::size_t amount) override;
  virtual int receive(char* data_buffer, std::size_t buffer_size) override;
  virtual int setBaudrate(Baudrate new_baudrate) override;

  void setUART(UART_HandleTypeDef* huart);
  UART_HandleTypeDef* UART() const;

  void setTimeout(std::uint32_t timeout);
  std::uint32_t timeout() const;

private:
  UART_HandleTypeDef* m_uart { nullptr };
  std::uint32_t m_timeout { HAL_MAX_DELAY };
};

}
