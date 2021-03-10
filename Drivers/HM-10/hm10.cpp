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
  if (!transmitAndCheckResponse("OK+RESET", "AT+RESET")) {
    return false;
  }

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
  if (transmitAndCheckResponse("OK+RENEW", "AT+RENEW")) {
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
  debugLog("Checking MAC address");

  if (transmitAndCheckResponse("OK+ADDR", "AT+ADDR?")) {
    // omitting OK+ADDR: with + 8
    copyStringFromResponse(8, addr.address);
  }

  return addr;
}

bool HM10::setMACAddress(char const* address) {
  debugLog("Setting MAC address to %s", address);
  return transmitAndCheckResponse("OK+Set", "AT+ADDR%s", address);
}

AdvertInterval HM10::advertisingInterval() {
  debugLog("Checking advertising interval");
  if (transmitAndCheckResponse("OK+Get", "AT+ADVI?")) {
    return static_cast<AdvertInterval>(extractNumberFromResponse(7, 16));
  }
  return AdvertInterval::InvalidInterval;
}

bool HM10::setAdvertisingInterval(AdvertInterval interval) {
  debugLog("Setting advertising interval to %d", static_cast<uint8_t>(interval));
  return transmitAndCheckResponse("OK+Set", "AT+ADVI%01X", static_cast<std::uint8_t>(interval));
}

AdvertType HM10::advertisingType() {
  debugLog("Checking advertising type");
  if (transmitAndCheckResponse("OK+Get", "AT+ADTY?")) {
    return static_cast<AdvertType>(extractNumberFromResponse());
  }
  return AdvertType::Invalid;
}

bool HM10::setAdvertisingType(AdvertType type) {
  debugLog("Setting advertising type to %d", static_cast<std::uint8_t>(type));
  return transmitAndCheckResponse("OK+Set", "AT+ADTY%d", static_cast<std::uint8_t>(type));
}

bool HM10::whiteListEnabled() {
  debugLog("Checking whitelist state");
  if (transmitAndCheckResponse("OK+Get", "AT+ALLO?")) {
    return static_cast<bool>(extractNumberFromResponse());
  } else {
    return false; // ¯\_(ツ)_/¯
  }
}

bool HM10::setWhiteListState(bool status) {
  debugLog("Setting whitelist state to %d", (status ? 1 : 0));
  return transmitAndCheckResponse("OK+Set", "AT+ALLO%d", (status ? 1 : 0));
}

MACAddress HM10::whiteListedMAC(std::uint8_t id) {
  MACAddress mac { };

  if (id < 1 || id > 3) {
    return mac;
  }

  debugLog("Checking whitelisted MAC #%d", id);

  if (transmitAndCheckResponse("OK+AD", "AT+AD%d??", id)) {
    copyStringFromResponse(8, mac.address);
  }
  return mac;
}

bool HM10::setWhitelistedMAC(std::uint8_t id, char const* address) {
  debugLog("Setting whitelisted MAC #%d to %s", id, address);
  return transmitAndCheckResponse("OK+AD", "AT+AD%d%s", id, address);
}

ConnInterval HM10::minimumConnectionInterval() {
  debugLog("Checking minimum connection interval");
  if (transmitAndCheckResponse("OK+Get", "AT+COMI?")) {
    return static_cast<ConnInterval>(extractNumberFromResponse());
  }
  return ConnInterval::InvalidInterval;
}

bool HM10::setMinimumConnectionInterval(ConnInterval interval) {
  debugLog("Setting minimum connection interval to %d", interval);
  return transmitAndCheckResponse("OK+Set", "AT+COMI%d", static_cast<std::uint8_t>(interval));
}

ConnInterval HM10::maximumConnectionInterval() {
  debugLog("Checking maximum connection interval");
  if (transmitAndCheckResponse("OK+Get", "AT+COMA?")) {
    return static_cast<ConnInterval>(extractNumberFromResponse());
  }
  return ConnInterval::InvalidInterval;
}

bool HM10::setMaximumConnectionInterval(ConnInterval interval) {
  debugLog("Setting maximum connection interval to %d", interval);
  return transmitAndCheckResponse("OK+Set", "AT+COMA%d", static_cast<std::uint8_t>(interval));
}

int HM10::connectionSlaveLatency() {
  debugLog("Checking connection slave latency");
  if (transmitAndCheckResponse("OK+Get", "AT+COLA?")) {
    return static_cast<int>(extractNumberFromResponse());
  }
  return -1;
}

bool HM10::setConnectionSlaveLatency(int latency) {
  if (latency < 0 || latency > 4) {
    return false;
  }

  debugLog("Setting connection slave latency to %d", latency);
  return transmitAndCheckResponse("OK+Set", "AT+COLA%d", static_cast<std::uint8_t>(latency));
}

ConnSupervisionTimeout HM10::connectionSupervisionTimeout() {
  debugLog("Checking connection supervision timeout");
  if (transmitAndCheckResponse("OK+Get", "AT+COSU?")) {
    return static_cast<ConnSupervisionTimeout>(extractNumberFromResponse());
  }
  return ConnSupervisionTimeout::InvalidTimeout;
}

bool HM10::setConnectionSupervisionTimeout(ConnSupervisionTimeout timeout) {
  debugLog("Setting connection supervision timeout to %d", static_cast<std::uint8_t>(timeout));
  return transmitAndCheckResponse("OK+Set", "AT+COSU%d", static_cast<std::uint8_t>(timeout));
}

bool HM10::updateConnection() {
  debugLog("Checking status of connection updating");
  if (transmitAndCheckResponse("OK+Get", "AT+COUP?")) {
    return static_cast<bool>(extractNumberFromResponse());
  }
  return false;
}

bool HM10::setConnectionUpdating(bool state) {
  debugLog("Setting connection updating to %d", static_cast<std::uint8_t>(state));
  return transmitAndCheckResponse("OK+Set", "AT+COUP%d", static_cast<std::uint8_t>(state));
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

bool HM10::transmitAndCheckResponse(char const* expectedResponse, char const* format, ...) {
  std::va_list args;
  va_start(args, format);
  copyCommandToBufferVarg(format, args);
  va_end(args);

  debugLog("Transmitting: %s", m_txBuffer);
  if (!transmitAndReceive()) {
    return false;
  }
  debugLog("Got response: %s", m_messageBuffer);

  return compareWithResponse(expectedResponse);
}

void HM10::copyCommandToBuffer(char const* const commandPattern, ...) {
  std::va_list args;
  va_start(args, commandPattern);

  copyCommandToBufferVarg(commandPattern, args);

  va_end(args);
}

void HM10::copyCommandToBufferVarg(char const* const commandPattern, std::va_list args) {
  m_txDataLength = vsnprintf(&m_txBuffer[0], bufferSize(), commandPattern, args);
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
