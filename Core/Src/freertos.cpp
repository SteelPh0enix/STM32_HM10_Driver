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
#include "printf.h"
#include "usart.h"
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
//HM10::HM10 hm10<64>(&hal_interface);

/* USER CODE END Variables */
/* Definitions for mainTask */
osThreadId_t mainTaskHandle;
osThreadAttr_t const mainTask_attributes = { .name = "mainTask", .stack_size = 512 * 4, .priority = // @suppress("Invalid arguments")
    (osPriority_t) osPriorityNormal, };

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void testFactoryReset();
void testBaudRate(HM10::Baudrate new_baud = HM10::Baudrate::Baud230400);
void testMACAddress(char const* new_mac = "");
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

  // TODO: idle-line based RX instead of standard DMA RX
  osDelay(100);

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
//  testFactoryReset();
//  testBaudRate();
  testMACAddress();

  printf("===== TESTS DONE! =====\n");
  for (;;) {
    osDelay(1);
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
  char mac[13];
  hm10.macAddress(mac);
  printf("Current MAC address: %s\n", mac);

  if (std::strlen(new_mac) == 12) {
    hm10.setMACAddress(new_mac);
    hm10.macAddress(mac);
    printf("New MAC address: %s\n", mac);
  }
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
