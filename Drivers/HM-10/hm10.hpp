/*
 * hm10.hpp
 *
 *  Created on: 4 mar 2021
 *      Author: SteelPh0enix
 */

/*
 * IMPORTANT: BEFORE YOU USE THIS LIBRARY, READ THIS
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
  HM10(UART_HandleTypeDef* uart);

  void setUART(UART_HandleTypeDef* uart);
  UART_HandleTypeDef* UART() const;

  std::size_t bufferSize() const;

  // Interrupt handler methods, use them to notify about RX/TX completion
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

  // Set the baudrate of the module and MCU
  bool setBaudRate(Baudrate new_baud);

private:
  int transmitBuffer();
  void waitForTransmitCompletion() const;

  int startReceivingToBuffer(std::size_t expected_bytes = HM10_BUFFER_SIZE);
  int receiveToBuffer(std::size_t expected_bytes = HM10_BUFFER_SIZE);
  void waitForReceiveCompletion() const;

  void copyCommandToBuffer(char const* const command);
  bool compareWithResponse(char const* str);

  UART_HandleTypeDef* m_uart { nullptr };

  char m_txBuffer[HM10_BUFFER_SIZE];
  std::size_t m_txDataLength { 0 };

  char m_rxBuffer[HM10_BUFFER_SIZE];
  std::size_t m_rxDataLength { 0 };

  bool m_rxInProgress { false };
  bool m_txInProgress { false };
};

}
