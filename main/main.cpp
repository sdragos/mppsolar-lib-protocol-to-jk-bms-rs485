#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "esphome/components/uart/uart_component_esp_idf.h"
#include "esphome/components/uart/uart.h"
#include "bms_lib_protocol_uart_handler.h"
#include "bms_lib_protocol_data_adapter.h"
#include "bms_lib_protocol_mock_data_adapter.h"

/**
 * This is a example which BMS_LIBs any data it receives on UART back to the sender using RS485 interface in half duplex mode.
*/
#define TAG "MPPSOLAR-RS485-LIBPROTOCOL-BMS"


#define USE_ESP_IDF

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins using Kconfig.
#define BMS_LIB_TXD   (CONFIG_BMS_LIB_UART_TXD)
#define BMS_LIB_RXD   (CONFIG_BMS_LIB_UART_RXD)

#define JK_TXD (CONFIG_JK_UART_TXD)
#define JK_RXD (CONFIG_JK_UART_RXD)

#define BMS_LIB_UART_BAUD_RATE       (CONFIG_BMS_LIB_UART_BAUD_RATE)
#define JK_UART_BAUD_RATE            (CONFIG_JK_UART_BAUD_RATE)

#define BUF_SIZE        (256)

// Read packet timeout
#define BMS_LIB_TASK_STACK_SIZE    (2048)
#define BMS_LIB_TASK_PRIO          (10)
#define BMS_LIB_UART_PORT          (CONFIG_BMS_LIB_UART_PORT_NUM)
#define JK_UART_PORT               (CONFIG_JK_UART_PORT_NUM)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged state on receive pin
// #define BMS_LIB_READ_TOUT          (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

using namespace esphome::uart;
using namespace sdragos::mppsolar;

static void uart_lib_protocol_slave(void *arg){
  IDFUARTComponent idf_uart;
  idf_uart.set_baud_rate(BMS_LIB_UART_BAUD_RATE);
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

static void uart_jk_master(void *arg){
  // Acts as a master, polls the JK BMS every 5 seconds for data so that it can be passed on the Lib protocol inverter on request.
  IDFUARTComponent idf_uart;
  idf_uart.set_baud_rate(JK_UART_BAUD_RATE);
  idf_uart.set_data_bits(8);
  idf_uart.set_stop_bits(1);
  idf_uart.set_parity(UARTParityOptions::UART_CONFIG_PARITY_NONE);
  idf_uart.set_rx_buffer_size(BUF_SIZE);
  idf_uart.set_tx_pin(new InternalGPIOPin(JK_TXD, false));
  idf_uart.set_rx_pin(new InternalGPIOPin(JK_RXD, false));

  idf_uart.setup();

  // instantiate the JK BMS protocol handler

  while (true){
    //todo: do stuff

    vTaskDelay(5000 / portTICK_PERIOD_MS); // wait 5 seconds
  }

  vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    //A uart read/write example without event queue;
    xTaskCreate(uart_lib_protocol_slave, "uart_lib_protocol_slave", BMS_LIB_TASK_STACK_SIZE, NULL, BMS_LIB_TASK_PRIO, NULL);
}
