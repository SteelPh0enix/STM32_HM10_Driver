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

// ===== Constructor, getters/setters, public utils ===== //

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

int HM10::initialize() {
  debugLog("HM10 init...\n");
  __HAL_UART_ENABLE_IT(UART(), UART_IT_IDLE);
  return HAL_UART_Receive_DMA(UART(), reinterpret_cast<uint8_t*>(&m_rxBuffer[0]), bufferSize());
}

void HM10::receiveCompleted() {
  char* const messageEndPtr = &m_rxBuffer[0]
                              + (bufferSize() - __HAL_DMA_GET_COUNTER(UART()->hdmarx));
  // debugLog("Bytes left in buffer: %d\n", __HAL_DMA_GET_COUNTER(UART()->hdmarx));

  // We need to check if DMA has rolled over the buffer during the rx procedure.
  // DMA counter rolls back to 128 when buffer ends, so there will never be a situation
  // where messageEndPtr is invalid (pointing to memory outside of the buffer).
  // At the same time, there should never be 0 characters in buffer so
  // checking for equality is pointless, yet i'm still gonna do that just-in-case so it's
  // not handled like split message.
  if (m_msgStartPtr <= messageEndPtr) {
    // DMA hasn't rolled over, message is in one, linear piece
    std::size_t const messageLength = messageEndPtr - m_msgStartPtr;
    std::memcpy(&m_messageBuffer[0], m_msgStartPtr, messageLength);
    m_messageBuffer[messageLength] = '\0';
    m_messageLength = messageLength;
  } else {
    // DMA has rolled over, copy two parts of the message into the buffer
    std::size_t const messagePrefixLength = m_rxBufferEnd - m_msgStartPtr;
    std::size_t const messageSuffixLength = messageEndPtr - &m_rxBuffer[0];
    std::memcpy(&m_messageBuffer[0], m_msgStartPtr, messagePrefixLength);
    std::memcpy(&m_messageBuffer[messagePrefixLength], &m_rxBuffer[0], messageSuffixLength);
    m_messageBuffer[messagePrefixLength + messageSuffixLength] = '\0';
    m_messageLength = messagePrefixLength + messageSuffixLength;
  }

  debugLog("Message received, length: %d, data: %s\n", m_messageLength, m_messageBuffer);
  m_msgStartPtr = messageEndPtr;
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
  copyCommandToBuffer("AT");

  if (!transmitAndReceive(100)) {
    return false;
  }

  return compareWithResponse("OK");
}

bool HM10::reboot(bool waitForStartup) {
  copyCommandToBuffer("AT+RESET");
  if (!transmitAndReceive()) {
    return false;
  }

  debugLog("[reboot] response: %s\n", m_messageBuffer);

  if (m_factoryRebootPending) {
    std::uint32_t const newBaudrate = BaudrateValues[static_cast<std::uint8_t>(DefaultBaudrate)];
    debugLog("[reboot] Factory reboot in progress, resetting UART baudrate to default (%d)\n",
             newBaudrate);
    setUARTBaudrate(newBaudrate);
    m_currentBaudrate = DefaultBaudrate;
    m_newBaudrate = DefaultBaudrate;
    m_factoryRebootPending = false;
  } else if (m_currentBaudrate != m_newBaudrate) {
    std::uint32_t const newBaudrate = BaudrateValues[static_cast<std::uint8_t>(m_newBaudrate)];
    debugLog("[reboot] rebooting after changing baud, new baud = %d\n", newBaudrate);
    setUARTBaudrate(newBaudrate);
    m_currentBaudrate = m_newBaudrate;
  }

  if (waitForStartup) {
    platformDelay(800); // should be booted up at this point
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
  if (!transmitAndReceive()) {
    return false;
  }

  if (compareWithResponse("OK+RENEW")) {
    m_factoryRebootPending = true;
    return reboot(waitForStartup);
  } else {
    return false;
  }
}

bool HM10::setBaudRate(Baudrate new_baud, bool rebootImmediately, bool waitForStartup) {
  m_txDataLength = sprintf(m_txBuffer, "AT+BAUD%d", static_cast<std::uint8_t>(new_baud));
  if (!transmitAndReceive()) {
    return false;
  }

  m_newBaudrate = new_baud;

  if (rebootImmediately) {
    return reboot(waitForStartup);
  }
  return compareWithResponse("OK+SET");
}

Baudrate HM10::baudRate() {
  return m_currentBaudrate;
}

// ===== Private/low-level/utility functions ===== //

int HM10::transmitBuffer() {
  m_txInProgress = true;
//  debugLog("[TX] %s\n", m_utilityBuffer);
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

void HM10::startReceivingToBuffer() {
  m_rxInProgress = true;
}

void HM10::abortReceiving() {
  m_rxInProgress = false;
}

bool HM10::waitForReceiveCompletion(std::uint32_t max_time) const {
  std::uint32_t counter { 0 };
  while (isReceiving() && max_time > counter) {
    platformDelay(1);
    counter++;
  }

  return max_time > counter;
}

bool HM10::receiveToBuffer() {
  startReceivingToBuffer();
  return waitForReceiveCompletion();
}

bool HM10::transmitAndReceive(std::uint32_t rx_wait_time) {
  startReceivingToBuffer();

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
  return std::strncmp(m_messageBuffer, str, std::strlen(str)) == 0;
}

void HM10::setUARTBaudrate(std::uint32_t new_baud) {
  UART()->Init.BaudRate = new_baud;
  HAL_UART_Init(UART());
}

}
