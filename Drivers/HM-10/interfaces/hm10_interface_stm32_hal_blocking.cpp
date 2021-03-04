/*
 * hm10_interface_stm32_hal_blocking.cpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#include "hm10_interface_stm32_hal_blocking.hpp"

HM10::InterfaceSTM32HALBlocking::InterfaceSTM32HALBlocking(UART_HandleTypeDef* huart,
                                                           std::uint32_t timeout) {
  setUART(huart);
  setTimeout(timeout);
}

int HM10::InterfaceSTM32HALBlocking::transmit(char* data, std::size_t amount) {
  return HAL_UART_Transmit(m_uart,
                           reinterpret_cast<uint8_t*>(data),
                           static_cast<uint16_t>(amount),
                           timeout());
}

int HM10::InterfaceSTM32HALBlocking::receive(char* data_buffer, std::size_t buffer_size) {
  return HAL_UART_Receive(m_uart,
                          reinterpret_cast<uint8_t*>(data_buffer),
                          static_cast<uint16_t>(buffer_size),
                          timeout());
}

int HM10::InterfaceSTM32HALBlocking::setBaudrate(Baudrate new_baudrate) {
  m_uart->Init.BaudRate = BaudrateValues[static_cast<std::uint8_t>(new_baudrate)];
  return HAL_UART_Init(m_uart);
}

void HM10::InterfaceSTM32HALBlocking::setUART(UART_HandleTypeDef* huart) {
  m_uart = huart;
}

UART_HandleTypeDef* HM10::InterfaceSTM32HALBlocking::UART() const {
  return m_uart;
}

void HM10::InterfaceSTM32HALBlocking::setTimeout(std::uint32_t timeout) {
  m_timeout = timeout;
}

std::uint32_t HM10::InterfaceSTM32HALBlocking::timeout() const {
  return m_timeout;
}
