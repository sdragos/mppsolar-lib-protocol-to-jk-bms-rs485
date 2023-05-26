#define USE_ESP_IDF
#define USE_ESP32
#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart_component_esp_idf.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/jk_modbus/jk_modbus.h"
#include "esphome/components/jk_bms/jk_bms.h"
#include "bms_lib_protocol_uart_handler.h"
#include "bms_lib_protocol_data_adapter.h"
#include "bms_lib_protocol_mock_data_adapter.h"

/**
 * This is a example which BMS_LIBs any data it receives on UART back to the sender using RS485 interface in half duplex mode.
 */
#define TAG "MPPSOLAR-RS485-LIBPROTOCOL-BMS"

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins using Kconfig.
#define MONITOR_BAUD (CONFIG_MONITOR_BAUD)

#define BMS_LIB_TXD (CONFIG_BMS_LIB_UART_TXD)
#define BMS_LIB_RXD (CONFIG_BMS_LIB_UART_RXD)

#define JK_TXD (CONFIG_JK_UART_TXD)
#define JK_RXD (CONFIG_JK_UART_RXD)

#define BMS_LIB_UART_BAUD_RATE (CONFIG_BMS_LIB_UART_BAUD_RATE)
#define JK_UART_BAUD_RATE (CONFIG_JK_UART_BAUD_RATE)

#define BUF_SIZE (256)

// Read packet timeout
#define BMS_LIB_TASK_STACK_SIZE (2048)
#define BMS_LIB_TASK_PRIO (10)
#define BMS_LIB_UART_PORT (CONFIG_BMS_LIB_UART_PORT_NUM)
#define JK_UART_PORT (CONFIG_JK_UART_PORT_NUM)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged state on receive pin
// #define BMS_LIB_READ_TOUT          (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

using namespace esphome;
using namespace esphome::uart;
using namespace esphome::jk_modbus;
using namespace esphome::jk_bms;
using namespace sdragos::mppsolar;

static esphome::jk_modbus::JkModbus *jkModbus_ = nullptr;
static esphome::jk_bms::JkBms *jkBms_ = nullptr;
static BMSLibProtocolUARTHandler *bmsLibProtocolUARTHandler_ = nullptr;

namespace esphome
{

  static void setup()
  {
    IDFUARTComponent *idf_uart_1 = new IDFUARTComponent();
    idf_uart_1->set_baud_rate(BMS_LIB_UART_BAUD_RATE);
    idf_uart_1->set_data_bits(8);
    idf_uart_1->set_stop_bits(1);
    idf_uart_1->set_parity(UARTParityOptions::UART_CONFIG_PARITY_NONE);
    idf_uart_1->set_rx_buffer_size(BUF_SIZE);
    idf_uart_1->set_tx_pin(new InternalGPIOPin(BMS_LIB_TXD, false));
    idf_uart_1->set_rx_pin(new InternalGPIOPin(BMS_LIB_RXD, false));

    idf_uart_1->setup();

    IDFUARTComponent *idf_uart_2 = new IDFUARTComponent();
    idf_uart_2->set_baud_rate(JK_UART_BAUD_RATE);
    idf_uart_2->set_data_bits(8);
    idf_uart_2->set_stop_bits(1);
    idf_uart_2->set_parity(UARTParityOptions::UART_CONFIG_PARITY_NONE);
    idf_uart_2->set_rx_buffer_size(BUF_SIZE);
    idf_uart_2->set_tx_pin(new InternalGPIOPin(JK_TXD, false));
    idf_uart_2->set_rx_pin(new InternalGPIOPin(JK_RXD, false));

    idf_uart_2->setup();

    ESP_LOGI(TAG, "UART setup done.\r\n");

    // instantiate the JK BMS protocol handler
    jkModbus_ = new esphome::jk_modbus::JkModbus();
    jkModbus_->set_uart_parent(idf_uart_2);

    ESP_LOGI(TAG, "JK Modbus setup done.\r\n");

    jkBms_ = new esphome::jk_bms::JkBms();
    jkBms_->set_parent(jkModbus_);
    jkBms_->set_enable_fake_traffic(true);

    ESP_LOGI(TAG, "JK BMS setup done.\r\n");

    jkModbus_->register_device(jkBms_);

    ESP_LOGI(TAG, "JK Modbus register device done.\r\n");

    jkModbus_->setup();

    ESP_LOGI(TAG, "JK Modbus setup done.\r\n");

    bmsLibProtocolUARTHandler_ = new BMSLibProtocolUARTHandler(idf_uart_1);

    auto mockDataAdapter = new BMSLibProtocolMockDataAdapter();
    bmsLibProtocolUARTHandler_->setDataAdapter(jkBms_);

    bmsLibProtocolUARTHandler_->setup();
  }

  static void uart_lib_protocol_slave(void *arg)
  {

    ESP_LOGI(TAG, "UART start receive loop.\r\n");

    while (true)
    {
      bmsLibProtocolUARTHandler_->loop();
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
  }

  static void uart_jk_master(void *arg)
  {
    // Acts as a master, polls the JK BMS every 5 seconds for data so that it can be passed on the Lib protocol inverter on request.
    TickType_t tickCount = xTaskGetTickCount();
    TickType_t previousUpdateWasAtTickCount = 0;
    TickType_t previousDumpConfigWasAtTickCount = 0;

    while (true)
    {
      jkModbus_->loop();

      tickCount = xTaskGetTickCount();
      if ((tickCount - previousUpdateWasAtTickCount) >= (5000 * portTICK_PERIOD_MS))
      {
        jkBms_->update();
        previousUpdateWasAtTickCount = tickCount;
      }

      if ((tickCount - previousDumpConfigWasAtTickCount) >= (30000 * portTICK_PERIOD_MS))
      {
        jkBms_->dump_config();
        previousDumpConfigWasAtTickCount = tickCount;
      }
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
  }

  extern "C" void app_main(void)
  {
    setup();
    // A uart read/write example without event queue;
    auto uartLibProtocolSlaveTaskHandle = xTaskCreate(uart_lib_protocol_slave, "uart_lib_protocol_slave", BMS_LIB_TASK_STACK_SIZE, NULL, BMS_LIB_TASK_PRIO, NULL);
    auto uartJkMasterTaskHandle = xTaskCreate(uart_jk_master, "uart_jk_master", BMS_LIB_TASK_STACK_SIZE, NULL, BMS_LIB_TASK_PRIO, NULL);

    ESP_LOGI(TAG, "Tasks created.\r\n");

    if (uartLibProtocolSlaveTaskHandle != pdPASS)
    {
      ESP_LOGE(TAG, "uart_lib_protocol_slave task create failed");
      return;
    }

    if (uartJkMasterTaskHandle != pdPASS)
    {
      ESP_LOGE(TAG, "uart_jk_master task create failed");
      return;
    }
  }
}
