#define USE_ESP_IDF
#define USE_ESP32
#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include<stdio.h>
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

#define TAG "Main"

#define MONITOR_BAUD (CONFIG_MONITOR_BAUD)

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins using Kconfig.
#define BMS_LIB_TXD (CONFIG_BMS_LIB_UART_TXD)
#define BMS_LIB_RXD (CONFIG_BMS_LIB_UART_RXD)
#define JK_TXD (CONFIG_JK_UART_TXD)
#define JK_RXD (CONFIG_JK_UART_RXD)

#define BMS_LIB_UART_BAUD_RATE (CONFIG_BMS_LIB_UART_BAUD_RATE)
#define JK_UART_BAUD_RATE (CONFIG_JK_UART_BAUD_RATE)

#define BMS_LIB_BUF_SIZE (256)
#define JK_BUF_SIZE (384)

// Read packet timeout
#define BMS_LIB_TASK_STACK_SIZE (32768)
#define BMS_LIB_TASK_PRIO (10)
#define BMS_LIB_IDF_UART_PORT (CONFIG_BMS_LIB_UART_PORT_NUM)
#define JK_IDF_UART_PORT (CONFIG_JK_UART_PORT_NUM)

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
    IDFUARTComponent *idf_uart_for_lib_protocol = new IDFUARTComponent();
    idf_uart_for_lib_protocol->set_baud_rate(BMS_LIB_UART_BAUD_RATE);
    idf_uart_for_lib_protocol->set_data_bits(8);
    idf_uart_for_lib_protocol->set_stop_bits(1);
    idf_uart_for_lib_protocol->set_parity(UARTParityOptions::UART_CONFIG_PARITY_NONE);
    idf_uart_for_lib_protocol->set_rx_buffer_size(BMS_LIB_BUF_SIZE);
    idf_uart_for_lib_protocol->set_tx_buffer_size(0);
    idf_uart_for_lib_protocol->set_event_queue_size(20);
    idf_uart_for_lib_protocol->set_tx_pin(new InternalGPIOPin(17, false));
    idf_uart_for_lib_protocol->set_rx_pin(new InternalGPIOPin(16, false));
    idf_uart_for_lib_protocol->set_uart_number(BMS_LIB_IDF_UART_PORT);

    idf_uart_for_lib_protocol->setup();

    IDFUARTComponent *idf_uart_for_jk_bms = new IDFUARTComponent();
    idf_uart_for_jk_bms->set_baud_rate(JK_UART_BAUD_RATE);
    idf_uart_for_jk_bms->set_data_bits(8);
    idf_uart_for_jk_bms->set_stop_bits(1);
    idf_uart_for_jk_bms->set_parity(UARTParityOptions::UART_CONFIG_PARITY_NONE);
    idf_uart_for_jk_bms->set_rx_buffer_size(JK_BUF_SIZE);
    idf_uart_for_jk_bms->set_tx_buffer_size(0);
    idf_uart_for_jk_bms->set_event_queue_size(20);
    idf_uart_for_jk_bms->set_tx_pin(new InternalGPIOPin(23, false));
    idf_uart_for_jk_bms->set_rx_pin(new InternalGPIOPin(22, false));
    idf_uart_for_jk_bms->set_uart_number(JK_IDF_UART_PORT);

    idf_uart_for_jk_bms->setup();

    ESP_LOGI(TAG, "UART setup done.\r\n");

    // instantiate the JK BMS protocol handler
    jkModbus_ = new esphome::jk_modbus::JkModbus();
    jkModbus_->set_uart_parent(idf_uart_for_jk_bms);
    jkModbus_->set_rx_timeout(100);

    ESP_LOGI(TAG, "JK Modbus setup done.\r\n");

    jkBms_ = new esphome::jk_bms::JkBms();
    jkBms_->set_parent(jkModbus_);
    jkBms_->set_address(0x4E);
    //jkBms_->set_enable_fake_traffic(true);

    ESP_LOGI(TAG, "JK BMS setup done.\r\n");

    jkModbus_->register_device(jkBms_);

    ESP_LOGI(TAG, "JK Modbus register device done.\r\n");

    jkModbus_->setup();

    ESP_LOGI(TAG, "JK Modbus setup done.\r\n");

    bmsLibProtocolUARTHandler_ = new BMSLibProtocolUARTHandler(idf_uart_for_lib_protocol);

    //auto mockDataAdapter = new BMSLibProtocolMockDataAdapter();
    bmsLibProtocolUARTHandler_->setDataAdapter(jkBms_);

    bmsLibProtocolUARTHandler_->setup();
  }

  extern "C" void app_main(void)
  {
    setup();

    ESP_LOGI(TAG, "UART start receive loop.\r\n");

    TickType_t tickCount = xTaskGetTickCount();
    TickType_t previousUpdateWasAtTickCount = 0;
    TickType_t previousDumpConfigWasAtTickCount = 0;

    TickType_t fiveSecondsTicks = 5000 / portTICK_PERIOD_MS;
    TickType_t thirtySecondsTicks = 30000 / portTICK_PERIOD_MS;
    
    jkBms_->update();
    
    
    while (true)
    {
      // In the loop methods the actual UART reads and writes happen.
      // These should return as quickly as possible to allow processing
      // needed by other components.
      bmsLibProtocolUARTHandler_->loop();
      jkModbus_->loop();

      tickCount = xTaskGetTickCount();
      if ((tickCount - previousUpdateWasAtTickCount) >= fiveSecondsTicks)
      {
        // Acts as a master, polls the JK BMS every 5 seconds for data so 
        // that it can be passed on the Lib protocol inverter on request.
        jkBms_->update();
        previousUpdateWasAtTickCount = tickCount;
      }

      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
  }
}
