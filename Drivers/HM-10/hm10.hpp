/*
 * hm10.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

/*
 * IMPORTANT: BEFORE YOU USE THIS LIBRARY, READ THIS
 * UPDATE YOUR HM-10 FIRMWARE BEFORE PLUGGING IT IN - THIS LIBRARY IS TESTED UNDER FIRMWARE V709
 * YOU CAN DOWNLOAD THE FIRMWARE AND UPDATE INSTRUCTIONS HERE: http://jnhuamao.cn/download_rom_en.asp?id=1
 *
 * This lib is created for usage with DMA, interrupts and RTOS. To use it without DMA, and/or without interrupts,
 * you have to re-write the `transmitBuffer` and `receiveToBuffer` methods. You'd also have to do that if you'd
 * want to port it to HAL_LL or registers, if you are into that kind of thing.
 * If you want to ditch RTOS, you're welcome to do so, but most of the functions in this library are blocking with RTOS delays.
 * This means that the recommended way to use this library is to create a thread solely for HM-10 communication, connect
 * it to the rest of the program with message queues, and let it run there. This lib will use osDelay to give CPU time to
 * other RTOS threads when it's busy waiting for HM-10 response. If you won't use RTOS, then the delays will block the whole CPU.
 *
 * Example application of this library should be in `example` branch of the official library repository.
 * Basically, you have to put some callbacks in UART interrupt handlers. Library will handle the rest.
 *
 * This library also uses idle-line UART interrupt, so if your hardware for some reason does not support it,
 * then you have to find a workaround (the only reasonable way is probably a timeout, which will drastically
 * decrease performance.
 */

#pragma once
#include <stm32f4xx.h>
#include <cstdint>
#include <cstdarg>
#include "hm10_constants.hpp"

// Re-define it somewhere in your code if you want to have different buffer size.
// I could use a template, but it'd be basically the same in most scenarios, and there's
// really no point in having different buffer sizes for each object - it's small enough.
#ifndef HM10_BUFFER_SIZE
#define HM10_BUFFER_SIZE 128
#endif

// Comment it out if you're not using RTOS (not recommended!)
#define USE_RTOS_DELAY

#ifdef USE_RTOS_DELAY
#include <cmsis_os2.h>
#define platformDelay osDelay
#else
#define platformDelay HAL_Delay
#endif

namespace HM10 {

class HM10 {
public:
  // V7xx firmware changed the default baudrate to 115200.
  // If your module doesn't respond after first plug-in and check, make sure
  // your firmware is updated.
  static constexpr Baudrate DefaultBaudrate { Baudrate::Baud115200 };

  HM10(UART_HandleTypeDef* uart);

  void setUART(UART_HandleTypeDef* uart);
  UART_HandleTypeDef* UART() const;

  std::size_t bufferSize() const;

  // Initializes the module communication (enables UART idle line interrupt and starts the receive procedure)
  // In order for this library to work correct, you have to call transmitCompleted IN IDLE LINE INTERRUPT HANDLER.
  // Since HAL does not support it out-of-the-box, you have to handle it manually in USARTx_IRQHandler().
  // RX DMA channel must run in circular mode.
  // See the example in repository (branch `example`) to see how to do that.
  int initialize();

  // Interrupt handler methods, use them to notify about RX/TX completion.
  // Call `receiveCompleted` in idle line interrupt handler.
  // Call `transmitCompleted` in standard transmit completion handler.
  void receiveCompleted();
  void transmitCompleted();

  // This function will return `true` if HM10 is busy doing something (haven't processed the command yet
  // or is in busy state).
  bool isBusy() const;

  // Similar to isBusy but returns the exact state of operation.
  bool isReceiving() const;
  bool isTransmitting() const;

  // Send `AT` to check if the module is alive and communication is working
  // Returns `true` if module responds OK, `false` on any error
  bool isAlive();

  // Soft-restart the module. Returns `true` if reboot was successfull, `false` on timeout.
  // If waitForStartup is `false`, then it returns `true` immediatelly after transmission is completed and successful.
  bool reboot(bool waitForStartup = true);

  // Will restore all the settings to factory defaults, along with baudrate of MCU UART (to 9600bps)
  bool factoryReset(bool waitForStartup = true);

  // Get/set the baudrate of the module and MCU UART. Setting it automatically reboots the module, unless `reboot` is false.
  Baudrate baudRate();
  bool setBaudRate(Baudrate new_baud, bool rebootImmediately = true, bool waitForStartup = true);

  // Get/set the MAC address of the module)
  MACAddress macAddress();
  bool setMACAddress(char const* address);

  // Get/set advertising interval
  AdvertInterval advertisingInterval();
  bool setAdvertisingInterval(AdvertInterval interval);

  // Get/set advertising type
  AdvertType advertisingType();
  bool setAdvertisingType(AdvertType type);

  // Get/set whitelist status
  bool whiteListEnabled();
  bool setWhiteListState(bool status);

  // Get/set whitelisted MAC address. HM-10 can have up to 3 addresses on whitelist, counting from 1.
  MACAddress whiteListedMAC(std::uint8_t id);
  bool setWhitelistedMAC(std::uint8_t id, char const* address);

  // Get/set minimum and maximum Link Layer connection interval
  ConnInterval minimumConnectionInterval();
  bool setMinimumConnectionInterval(ConnInterval interval);
  ConnInterval maximumConnectionInterval();
  bool setMaximumConnectionInterval(ConnInterval interval);

  // Get/set Link Layer connection slave latency (range 0 - 4)
  int connectionSlaveLatency();
  bool setConnectionSlaveLatency(int latency);

  // Get/set connection supervision timeout
  ConnSupervisionTimeout connectionSupervisionTimeout();
  bool setConnectionSupervisionTimeout(ConnSupervisionTimeout timeout);

  // Get/set the state of connection updating (slave mode)
  bool updateConnection();
  bool setConnectionUpdating(bool state);

  // Get/set characteristic value
  std::uint16_t characteristicValue();
  bool setCharacteristicValue(std::uint16_t value);

  // Get/set notifications state
  bool notificationsState();
  bool setNotificationsState(bool enabled);

  // Get/set notifications mode
  bool notificationsWithAddress();
  bool setNotificationsWithAddressState(bool enabled);

  // Clear last connected device address
  bool clearLastConnected();

  // Remove bond information
  bool removeBondInformation();

  // Get/set characteristics amount.
  // Second char can have address of `default char + 1` (SecondCharNext)
  // or `default char - 1` (SecondCharBefore)
  CharsAmount getCharacteristicsAmount();
  bool setCharacteristicsAmount(CharsAmount amount);

  // Get/set RX gain
  bool rxGain();
  bool setRXGain(bool open);

  // Get/set automatic work mode state
  // If enabled - module will start working immediatelly
  // If disabled, it'll only respond to AT and the comms will have to be
  // handled manually
  bool automaticMode();
  bool setAutomaticMode(bool enabled);

  // Get/set work mode
  WorkMode workMode();
  bool setWorkMode(WorkMode new_mode);

private:
  int transmitBuffer();
  void waitForTransmitCompletion() const;

  void startReceivingToBuffer();
  void abortReceiving();
  bool receiveToBuffer();
  bool waitForReceiveCompletion(std::uint32_t max_time = 1000) const;

  bool transmitAndReceive(std::uint32_t rx_wait_time = 1000);
  bool transmitAndCheckResponse(char const* expectedResponse, char const* format, ...);

  void copyCommandToBuffer(char const* const commandPattern, ...);
  void copyCommandToBufferVarg(char const* const commandPattern, std::va_list args);
  bool compareWithResponse(char const* str) const;

  // Most of the command responses are OK+Get: so the default offset it 7
  long extractNumberFromResponse(std::size_t offset = 7, int base = 10) const;
  void copyStringFromResponse(std::size_t offset, char* destination) const;

  void setUARTBaudrate(std::uint32_t new_baud) const;

  UART_HandleTypeDef* m_uart { nullptr };

  // TX buffer - data to transmit will be temporarily stored here
  char m_txBuffer[HM10_BUFFER_SIZE] { };
  std::size_t m_txDataLength { 0 };

  // RX buffer - for DMA, it'll put the data here
  char m_rxBuffer[HM10_BUFFER_SIZE] { };
  // Message buffer - the message from RX buffer will be copied here.
  // This is necessary for message reconstruction if DMA will roll over the buffer
  // while receiving the data.
  char m_messageBuffer[HM10_BUFFER_SIZE] { };
  std::size_t m_messageLength { 0 };
  char* m_msgStartPtr { &m_rxBuffer[0] };
  char const* m_rxBufferEnd { &m_rxBuffer[0] + HM10_BUFFER_SIZE };

  bool m_rxInProgress { false };
  bool m_txInProgress { false };

  bool m_factoryRebootPending { false };

  Baudrate m_currentBaudrate { DefaultBaudrate };
  Baudrate m_newBaudrate { DefaultBaudrate };
};

}
