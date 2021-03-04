/*
 * hm10.cpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#include "hm10.hpp"
#include <cstring>

namespace HM10 {

// ===== Constructor, getters/setters ===== //

HM10::HM10(UART_HandleTypeDef* uart) {
  if (uart != nullptr) {
    setUART(uart);
  }
}

void HM10::setUART(UART_HandleTypeDef* uart) {
  m_uart = uart;
}

UART_HandleTypeDef* HM10::UART() const {
  return m_uart;
}

std::size_t HM10::bufferSize() const {
  return HM10_BUFFER_SIZE;
}

void HM10::receiveCompleted() {
  m_rxInProgress = false;
}

void HM10::transmitCompleted() {
  m_txInProgress = false;
}

bool HM10::isBusy() const {
  return (isReceiving() || isTransmitting());
}

bool HM10::isReceiving() const {
  return m_rxInProgress;
}

bool HM10::isTransmitting() const {
  return m_txInProgress;
}

// ===== Functionality ===== //

bool HM10::isAlive() {
  copyCommandToBuffer(Command::AT);
  startReceivingToBuffer(2);

  if (transmitBuffer() != 0) {
    return false;
  }

  waitForReceiveCompletion();

  return compareWithResponse(Response::OK);
}

bool HM10::reboot(bool waitForStartup) {

}

bool HM10::setBaudRate(Baudrate new_baud) {

}

// ===== Private/low-level/utility functions ===== //

int HM10::transmitBuffer() {
  m_txInProgress = true;
  int transmit_result = HAL_UART_Transmit_DMA(m_uart,
                                              reinterpret_cast<uint8_t*>(&m_txBuffer[0]),
                                              m_txDataLength);
  if (transmit_result == HAL_OK) {
    waitForTransmitCompletion();
  } else {
    transmitCompleted();
  }

  return transmit_result;
}

void HM10::waitForTransmitCompletion() const {
  while (isTransmitting()) {
    platformDelay(1);
  }
}

int HM10::startReceivingToBuffer(std::size_t expected_bytes) {
  m_rxInProgress = true;
  return HAL_UART_Receive_DMA(m_uart, reinterpret_cast<uint8_t*>(m_rxBuffer), expected_bytes);
}

void HM10::waitForReceiveCompletion() const {
  while (isReceiving()) {
    platformDelay(1);
  }
}

int HM10::receiveToBuffer(std::size_t expected_bytes) {
  m_rxInProgress = true;
  int receive_result = HAL_UART_Receive_DMA(m_uart,
                                            reinterpret_cast<uint8_t*>(m_rxBuffer),
                                            expected_bytes);
  if (receive_result == HAL_OK) {
    waitForReceiveCompletion();
  } else {
    receiveCompleted();
  }

  return receive_result;
}

void HM10::copyCommandToBuffer(char const* const command) {
  std::strcpy(&m_txBuffer[0], command);
  m_txDataLength = std::strlen(command);
}

bool HM10::compareWithResponse(char const* str) {
  return std::strcmp(m_rxBuffer, str) == 0;
}

}
