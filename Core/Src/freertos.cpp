/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "printf.h"
#include <hm10.hpp>
#include <cstring>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HM10_UART huart1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
HM10::HM10 hm10(&HM10_UART);
char hm_message_buffer[128] { };
bool message_received = false;

/* USER CODE END Variables */
/* Definitions for mainTask */
osThreadId_t mainTaskHandle;
osThreadAttr_t const mainTask_attributes = { .name = "mainTask", .stack_size = 512 * 4, .priority =
    (osPriority_t) osPriorityNormal, };

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void testFactoryReset();
void testBaudRate(HM10::Baudrate new_baud = HM10::Baudrate::Baud115200);
void testMACAddress(char const* new_mac = "");
void testAdvertisingInterval(HM10::AdvertInterval new_interval = HM10::AdvertInterval::Adv546p25ms);
void testMACWhitelist();
void testConnectionIntervals();
void testSlaveLatency();
void testSupervisionTimeout();
void testConnectionUpdating();
void testCharacteristicValue();
void testNotifications();

void dataCallback(char* data, std::size_t length);
void connectedCallback(HM10::MACAddress const& mac);
void disconnectedCallback();
/* USER CODE END FunctionPrototypes */

void StartMainTask(void* argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of mainTask */
  mainTaskHandle = osThreadNew(StartMainTask, NULL, &mainTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartMainTask */
/**
 * @brief  Function implementing the mainTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartMainTask */
void StartMainTask(void* argument) {
  /* USER CODE BEGIN StartMainTask */

  osDelay(1000);

  int initStatus = hm10.initialize();
  if (initStatus != HAL_OK) {
    printf("An error occurred while initializing HM10 comms: %d (0x%02X)\n",
           initStatus,
           initStatus);
  }

  bool isAlive { false };
  while (!isAlive) {
    isAlive = hm10.isAlive();
    printf("Is alive? %s\n", (hm10.isAlive() ? "yes" : "no"));
    osDelay(100);
  }

  printf("===== TESTS STARTING =====\n");

  testFactoryReset();
  testBaudRate();
//  testMACAddress();
//  testAdvertisingInterval();
//  testMACWhitelist();
//  testConnectionIntervals();
//  testSlaveLatency();
//  testSupervisionTimeout();
//  testConnectionUpdating();
//  testCharacteristicValue();
//  testNotifications();

  HM10::Version version = hm10.firmwareVersion();
  printf("Firmware version: %s\n", version.version);

  hm10.setDataCallback(dataCallback);
  hm10.setDeviceConnectedCallback(connectedCallback);
  hm10.setDeviceDisconnectedCallback(disconnectedCallback);

  hm10.setAutomaticMode(true);
  printf("Automatic mode: %s\n", hm10.automaticMode() ? "enabled" : "disabled");

  hm10.setAutoSleep(false);
  printf("Auto sleep: %s\n", hm10.autoSleep() ? "enabled" : "disabled");

  hm10.setWorkMode(HM10::WorkMode::Transmission);
  printf("Work mode: %d\n", static_cast<int>(hm10.workMode()));

  hm10.setRole(HM10::Role::Peripheral);
  printf("Role: %d\n", static_cast<int>(hm10.role()));

  hm10.setBondingMode(HM10::BondMode::AuthNoPin);
  printf("Bonding mode: %d\n", static_cast<int>(hm10.bondingMode()));

  HM10::DeviceName name = hm10.name();
  printf("Name: %s\n", name.name);

  hm10.setName("hm10test");
  name = hm10.name();
  printf("New name: %s\n", name.name);

//  hm10.setPassword(123456);
//  printf("Password: %06d\n", hm10.password());

  hm10.setServiceUUID(0xDEAD);
  printf("Service UUID: 0x%04X\n", hm10.serviceUUID());

  hm10.setCharacteristicValue(0xBEEF);
  printf("Characteristic value: 0x%04X\n", hm10.characteristicValue());

  hm10.setNotificationsState(true);
  printf("Notifications state: %s\n", hm10.notificationsState() ? "enabled" : "disabled");

  hm10.setNotificationsWithAddressState(true);
  printf("Notifications with address: %s\n",
         hm10.notificationsWithAddress() ? "enabled" : "disabled");

  printf("===== TESTS DONE! =====\n");

  char const* response = "test response";

  for (;;) {
    osDelay(1);
    if (message_received) {
      printf("%d BYTES FROM MASTER: %s\n", std::strlen(hm_message_buffer), hm_message_buffer);
      hm10.sendData((uint8_t const*) response, std::strlen(response));
      message_received = false;
    }
  }
  /* USER CODE END StartMainTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void _putchar(char character) {
  if (character == '\n') {
    ITM_SendChar('\r');
  }
  ITM_SendChar(character);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  if (huart == &HM10_UART) {
    HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_RESET);
    hm10.transmitCompleted();
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
  if (huart == &HM10_UART) {
    printf("UART error - code %d (0x%02X)\n", huart->ErrorCode, huart->ErrorCode);
  }
}

extern "C" void HM10_UART_HandleIdleLine() {
  if (__HAL_UART_GET_FLAG(&HM10_UART, UART_FLAG_IDLE)) {
    __HAL_UART_CLEAR_IDLEFLAG(&HM10_UART);
    HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);
    hm10.receiveCompleted();
  }
}

void testFactoryReset() {
  hm10.factoryReset();
  printf("Is alive after factory reboot? %s\n", (hm10.isAlive() ? "yes" : "no"));
}

void testBaudRate(HM10::Baudrate new_baud) {
  hm10.setBaudRate(new_baud);

  printf("Is alive after baudrate change? %s\n", (hm10.isAlive() ? "yes" : "no"));
}

void testMACAddress(char const* new_mac) {
  HM10::MACAddress address = hm10.macAddress();
  printf("Current MAC address: %s\n", address.address);

  if (std::strlen(new_mac) == 12) {
    hm10.setMACAddress(new_mac);
    address = hm10.macAddress();
    printf("New MAC address: %s\n", address.address);
  }
}

void testAdvertisingInterval(HM10::AdvertInterval new_interval) {
  printf("Current adv interval: 0x%01X\n", hm10.advertisingInterval());

  if (new_interval != HM10::AdvertInterval::InvalidInterval) {
    hm10.setAdvertisingInterval(new_interval);
    printf("New adv interval: 0x%01X\n", hm10.advertisingInterval());
  }
}

void testMACWhitelist() {
  printf("Current MAC whitelist state: %s\n", hm10.whiteListEnabled() ? "On" : "Off");
  hm10.setWhiteListState(true);
  printf("New MAC whitelist state: %s\n", hm10.whiteListEnabled() ? "On" : "Off");
  hm10.setWhiteListState(false);

  for (int i = 1; i < 3; i++) {
    printf("Whitelisted MAC #%d: %s\n", i, hm10.whiteListedMAC(i).address);
  };
  hm10.setWhitelistedMAC(1, "AABBCCDDEEFF");
  hm10.setWhitelistedMAC(2, "112233445566");

  printf("Whitelisted MACs set!\n");
  for (int i = 1; i < 3; i++) {
    printf("Whitelisted MAC #%d: %s\n", i, hm10.whiteListedMAC(i).address);
  };
}

void testConnectionIntervals() {
  printf("Current connection intervals: %d to %d\n",
         static_cast<int>(hm10.minimumConnectionInterval()),
         static_cast<int>(hm10.maximumConnectionInterval()));

  hm10.setMinimumConnectionInterval(HM10::ConnInterval::Interval7p5ms);
  hm10.setMaximumConnectionInterval(HM10::ConnInterval::Interval45ms);

  printf("New connection intervals: %d to %d\n",
         static_cast<int>(hm10.minimumConnectionInterval()),
         static_cast<int>(hm10.maximumConnectionInterval()));
}

void testSlaveLatency() {
  printf("Current slave latency: %d\n", hm10.connectionSlaveLatency());
  hm10.setConnectionSlaveLatency(2);
  printf("New slave latency: %d\n", hm10.connectionSlaveLatency());
}

void testSupervisionTimeout() {
  printf("Current supervision timeout: %d\n",
         static_cast<int>(hm10.connectionSupervisionTimeout()));
  hm10.setConnectionSupervisionTimeout(HM10::ConnSupervisionTimeout::Timeout2000ms);
  printf("New supervision timeout: %d\n", static_cast<int>(hm10.connectionSupervisionTimeout()));
}

void testConnectionUpdating() {
  printf("Current connection updating status: %s\n", hm10.updateConnection() ? "true" : "false");
  hm10.setConnectionUpdating(false);
  printf("Current connection updating status: %s\n", hm10.updateConnection() ? "true" : "false");
}

void testCharacteristicValue() {
  printf("Current characteristic value: 0x%04X\n", hm10.characteristicValue());
  hm10.setCharacteristicValue(0xABCD);
  printf("New characteristic value: 0x%04X\n", hm10.characteristicValue());
}

void testNotifications() {
  printf("Current notifications mode: %s (%s address)\n",
         (hm10.notificationsState() ? "enabled" : "disabled"),
         (hm10.notificationsWithAddress() ? "with" : "without"));
  hm10.setNotificationsState(true);
  hm10.setNotificationsWithAddressState(true);
  printf("New notifications mode: %s (%s address)\n",
         (hm10.notificationsState() ? "enabled" : "disabled"),
         (hm10.notificationsWithAddress() ? "with" : "without"));
}

void dataCallback(char* data, std::size_t length) {
  std::memcpy(hm_message_buffer, data, length);
  message_received = true;
}

void connectedCallback(HM10::MACAddress const& mac) {
  printf("Connected to master with MAC address %s\n", mac.address);
}

void disconnectedCallback() {
  printf("Disconnected from master!\n");
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
