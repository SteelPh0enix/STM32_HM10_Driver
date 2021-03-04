/*
 * hm10.cpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#include "hm10.hpp"
#include <cstring>
#include "printf.h"

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
  m_rxDataLength = m_rxDataExpected - __HAL_DMA_GET_COUNTER(UART()->hdmarx);
  if (m_rxDataExpected < bufferSize()) {
    m_rxBuffer[m_rxDataLength] = '\0';
  }
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
  copyCommandToBuffer("AT");

  if (!transmitAndReceive(2, 100)) {
    return false;
  }

  return compareWithResponse("OK");
}

bool HM10::reboot(bool waitForStartup) {
  copyCommandToBuffer("AT+RESET");
  if (!transmitAndReceive(8)) {
    return false;
  }

  debugLog("[reboot] response: %s\n", m_rxBuffer);

  if (m_factoryRebootPending) {
    debugLog("[reboot] Factory reboot in progress, resetting UART baudrate to %d\n",
             DefaultBaudrate);
    setUARTBaudrate(DefaultBaudrate);
    m_factoryRebootPending = false;
  }

  if (waitForStartup) {
    platformDelay(700); // should be booted up at this point
    debugLog("[reboot] starting check-alive loop\n");
    while (!isAlive()) {
      platformDelay(100);
    }
  }

  debugLog("[reboot] Reboot successfull!\n");
  return true;
}

bool HM10::factoryReset(bool waitForStartup) {
  copyCommandToBuffer("AT+RENEW");
  if (!transmitAndReceive(8)) {
    return false;
  }

  if (compareWithResponse("OK+RENEW")) {
    m_factoryRebootPending = true;
    return reboot(waitForStartup);
  } else {
    return false;
  }
}

bool HM10::setBaudRate(Baudrate new_baud) {
  m_txDataLength = sprintf(m_txBuffer, "AT+BAUD%d", static_cast<std::uint8_t>(new_baud));
  if (!transmitAndReceive(8)) {
    return false;
  }

  return compareWithResponse("OK+SET");
}

Baudrate HM10::baudRate() {

}

// ===== Private/low-level/utility functions ===== //

int HM10::transmitBuffer() {
  m_txInProgress = true;
//  debugLog("[TX] %s\n", m_txBuffer);
  int transmit_result = HAL_UART_Transmit_DMA(UART(),
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
  m_rxDataExpected = expected_bytes;
  return HAL_UART_Receive_DMA(UART(), reinterpret_cast<uint8_t*>(m_rxBuffer), expected_bytes);
}

void HM10::abortReceiving() {
  m_rxInProgress = false;
  HAL_UART_AbortReceive(UART());
}

bool HM10::waitForReceiveCompletion(std::uint32_t max_time) const {
  std::uint32_t counter { 0 };
  while (isReceiving() && max_time > counter) {
    platformDelay(1);
    counter++;
  }

//  debugLog("[RX %d bytes] %s\n", m_rxDataLength, m_rxBuffer);
  return max_time > counter;
}

int HM10::receiveToBuffer(std::size_t expected_bytes) {
  int receive_result = startReceivingToBuffer(expected_bytes);

  if (receive_result == HAL_OK) {
    waitForReceiveCompletion();
  } else {
    receiveCompleted();
  }

  return receive_result;
}

bool HM10::transmitAndReceive(std::size_t expected_bytes, std::uint32_t rx_wait_time) {
  if (startReceivingToBuffer(expected_bytes) != 0) {
    debugLog("[TxRx] startRx error\n");
    abortReceiving();
    return false;
  }

  if (transmitBuffer() != 0) {
    debugLog("[TxRx] TX error\n");
    return false;
  }

  if (waitForReceiveCompletion(rx_wait_time) == false) {
    debugLog("[TxRx] RX timeout\n");
    return false;
  }

  return true;
}

void HM10::copyCommandToBuffer(char const* const command) {
  std::strcpy(&m_txBuffer[0], command);
  m_txDataLength = std::strlen(command);
  m_txBuffer[m_txDataLength] = '\0';
}

bool HM10::compareWithResponse(char const* str) {
  return std::strncmp(m_rxBuffer, str, std::strlen(str)) == 0;
}

void HM10::setUARTBaudrate(std::uint32_t new_baud) {
  UART()->Init.BaudRate = new_baud;
  HAL_UART_Init(UART());
}

}
