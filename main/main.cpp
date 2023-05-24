#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "uart_idf.h"
#include "uart_device.h"
#include "BMSLibProtocolUARTHandler.h"
#include "BMSLibProtocolDataAdapter.h"
#include "BMSLibProtocolMockDataAdapter.h"

/**
 * This is a example which BMS_LIBs any data it receives on UART back to the sender using RS485 interface in half duplex mode.
*/
#define TAG "MPPSOLAR-RS485-LIBPROTOCOL-BMS"


#define USE_ESP_IDF

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins using Kconfig.
#define BMS_LIB_TXD   (CONFIG_BMS_LIB_UART_TXD)
#define BMS_LIB_RXD   (CONFIG_BMS_LIB_UART_RXD)

// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define BMS_LIB_RTS   (CONFIG_BMS_LIB_UART_RTS)

// CTS is not used in RS485 Half-Duplex Mode
#define BMS_LIB_CTS   (UART_PIN_NO_CHANGE)

#define BAUD_RATE       (CONFIG_BMS_LIB_UART_BAUD_RATE)
#define BUF_SIZE        (256)

// Read packet timeout
#define BMS_LIB_TASK_STACK_SIZE    (2048)
#define BMS_LIB_TASK_PRIO          (10)
#define BMS_LIB_UART_PORT          (CONFIG_BMS_LIB_UART_PORT_NUM)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged state on receive pin
#define BMS_LIB_READ_TOUT          (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

using namespace esphome::uart;
using namespace sdragos::mppsolar;

static void uart_idf_test(void *arg){
  IDFUARTComponent idf_uart;
  idf_uart.set_baud_rate(BAUD_RATE);
  idf_uart.set_data_bits(8);
  idf_uart.set_stop_bits(1);
  idf_uart.set_parity(UARTParityOptions::UART_CONFIG_PARITY_NONE);
  idf_uart.set_rx_buffer_size(BUF_SIZE);
  idf_uart.set_tx_pin(new InternalGPIOPin(BMS_LIB_TXD, false));
  idf_uart.set_rx_pin(new InternalGPIOPin(BMS_LIB_RXD, false));

  idf_uart.setup();

  BMSLibProtocolUARTHandler bmsLibProtocolUARTHandler(&idf_uart);
  bmsLibProtocolUARTHandler.setDataAdapter(new BMSLibProtocolMockDataAdapter());
  bmsLibProtocolUARTHandler.setup();
  
  ESP_LOGI(TAG, "UART start receive loop.\r\n");
  
  while(true)
  {
    bmsLibProtocolUARTHandler.loop();
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    //A uart read/write example without event queue;
    xTaskCreate(uart_idf_test, "uart_idf_test", BMS_LIB_TASK_STACK_SIZE, NULL, BMS_LIB_TASK_PRIO, NULL);
}
