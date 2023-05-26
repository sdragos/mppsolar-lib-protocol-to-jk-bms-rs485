//This file contains code taken from https://github.com/esphome/esphome/blob/dev/esphome/components/uart/uart_component_esp_idf.h
//It is likely not a 1 to 1 copy as some things may have been modified so that it works without other implementations from ESPHome

#pragma once

#include <driver/uart.h>
#include "uart_component.h"

namespace esphome
{
  namespace uart
  {

    class IDFUARTComponent : public UARTComponent, public Component
    {
    public:
      void loop() override { };

      void setup() override;
      void dump_config() override;
      float get_setup_priority() const override { return esphome::setup_priority::BUS; }

      void write_array(const uint8_t *data, size_t len) override;

      bool peek_byte(uint8_t *data) override;
      bool read_array(uint8_t *data, size_t len) override;

      int available() override;
      void flush() override;

      uint8_t get_hw_serial_number() { return this->uart_num_; }
      QueueHandle_t *get_uart_event_queue() { return &this->uart_event_queue_; }

    protected:
      uart_port_t uart_num_;
      QueueHandle_t uart_event_queue_;
      uart_config_t get_config_();
      SemaphoreHandle_t lock_;

      bool has_peek_{false};
      uint8_t peek_byte_;
    };

  } // namespace uart
} // namespace esphome
