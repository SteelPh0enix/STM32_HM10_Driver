/*
 * hm10.cpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

#include "hm10.hpp"
#include "hm10_debug.hpp"
#include "printf.h"

#include <cstring>
#include <cstdlib>
#include <cstdarg>

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
  debugLog("Init started");
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

  debugLogLL("Message received, length: %d, data: %s", m_messageLength, m_messageBuffer);
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

  debugLog("Response: %s", m_messageBuffer);

  if (m_factoryRebootPending) {
    std::uint32_t const newBaudrate = BaudrateValues[static_cast<std::uint8_t>(DefaultBaudrate)];
    debugLog("Factory reboot in progress, resetting UART baudrate to default (%d)", newBaudrate);
    setUARTBaudrate(newBaudrate);
    m_currentBaudrate = DefaultBaudrate;
    m_newBaudrate = DefaultBaudrate;
    m_factoryRebootPending = false;
  } else if (m_currentBaudrate != m_newBaudrate) {
    std::uint32_t const newBaudrate = BaudrateValues[static_cast<std::uint8_t>(m_newBaudrate)];
    debugLog("Rebooting after changing baud, new baud = %d", newBaudrate);
    setUARTBaudrate(newBaudrate);
    m_currentBaudrate = m_newBaudrate;
  }

  if (waitForStartup) {
    platformDelay(800); // should be booted up at this point
    debugLog("Starting check-alive loop");
    while (!isAlive()) {
      platformDelay(100);
    }
  }

  debugLog("Reboot successfull!");
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
  copyCommandToBuffer("AT+BAUD%d", static_cast<std::uint8_t>(new_baud));
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

MACAddress HM10::macAddress() {
  MACAddress addr { };

  copyCommandToBuffer("AT+ADDR?");
  if (!transmitAndReceive()) {
    return addr;
  }

  if (!compareWithResponse("OK+ADDR")) {
    return addr;
  }

// omitting OK+ADDR: with + 8
  copyStringFromResponse(8, addr.address);
  return addr;
}

bool HM10::setMACAddress(char const* address) {
  copyCommandToBuffer("AT+ADDR%s", address);
  if (!transmitAndReceive()) {
    return false;
  }

  return compareWithResponse("OK+Set");
}

AdvertInterval HM10::advertisingInterval() {
  copyCommandToBuffer("AT+ADVI?");
  if (!transmitAndReceive()) {
    return AdvertInterval::InvalidInterval;
  }

  if (!compareWithResponse("OK+Get")) {
    return AdvertInterval::InvalidInterval;
  }

  return static_cast<AdvertInterval>(extractNumberFromResponse(7, 16));
}

bool HM10::setAdvertisingInterval(AdvertInterval interval) {
  copyCommandToBuffer("AT+ADVI%01X", static_cast<std::uint8_t>(interval));
  if (!transmitAndReceive()) {
    return false;
  }

  return compareWithResponse("OK+Set");
}

AdvertType HM10::advertisingType() {
  copyCommandToBuffer("AT+ADTY?");
  if (!transmitAndReceive()) {
    return AdvertType::Invalid;
  }

  if (!compareWithResponse("OK+Get")) {
    return AdvertType::Invalid;
  }

  return static_cast<AdvertType>(extractNumberFromResponse());
}

bool HM10::setAdvertisingType(AdvertType type) {
  copyCommandToBuffer("AT+ADTY%d", static_cast<std::uint8_t>(type));
  if (!transmitAndReceive()) {
    return false;
  }

  return compareWithResponse("OK+Set");
}

bool HM10::whiteListEnabled() {
  copyCommandToBuffer("AD+ALLO?");
  if (!transmitAndReceive()) {
    return false;
  }

  if (!compareWithResponse("OK+Get")) {
    return false;
  }

  return static_cast<bool>(extractNumberFromResponse());
}

bool HM10::setWhiteListStatus(bool status) {
  copyCommandToBuffer("AD+ALLO%d", (status ? 1 : 0));
  if (!transmitAndReceive()) {
    return false;
  }

  return compareWithResponse("OK+Set");
}

// ===== Private/low-level/utility functions ===== //

int HM10::transmitBuffer() {
  m_txInProgress = true;
  debugLogLL("Transmitting %s", m_txBuffer);
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
    debugLogLL("TX error");
    return false;
  }

  if (waitForReceiveCompletion(rx_wait_time) == false) {
    debugLogLL("RX timeout");
    return false;
  }

  return true;
}

void HM10::copyCommandToBuffer(char const* const command, ...) {
  std::va_list args;
  va_start(args, command);

  m_txDataLength = vsnprintf(&m_txBuffer[0], bufferSize(), command, args);

  va_end(args);
}

bool HM10::compareWithResponse(char const* str) const {
  return std::strncmp(m_messageBuffer, str, std::strlen(str)) == 0;
}

long HM10::extractNumberFromResponse(std::size_t offset, int base) const {
  return std::strtol(&m_messageBuffer[0] + offset, nullptr, base);
}

void HM10::copyStringFromResponse(std::size_t offset, char* destination) const {
  std::strcpy(destination, &m_messageBuffer[0] + offset);
}

void HM10::setUARTBaudrate(std::uint32_t new_baud) const {
  UART()->Init.BaudRate = new_baud;
  HAL_UART_Init(UART());
}

}
