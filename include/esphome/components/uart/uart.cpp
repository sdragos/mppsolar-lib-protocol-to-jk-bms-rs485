//This file contains code taken from https://github.com/esphome/esphome/blob/dev/esphome/components/uart/uart.cpp
//It is likely not a 1 to 1 copy as some things may have been modified so that it works without other implementations from ESPHome

#include "uart.h"

namespace esphome {
namespace uart {

static const char *const TAG = "uart";

void UARTDevice::check_uart_settings(uint32_t baud_rate, uint8_t stop_bits, UARTParityOptions parity,
                                     uint8_t data_bits) {
  if (this->parent_->get_baud_rate() != baud_rate) {
    ESP_LOGE(TAG, "  Invalid baud_rate: Integration requested baud_rate %lu but you have %lu!", baud_rate,
             this->parent_->get_baud_rate());
  }
  if (this->parent_->get_stop_bits() != stop_bits) {
    ESP_LOGE(TAG, "  Invalid stop bits: Integration requested stop_bits %u but you have %u!", stop_bits,
             this->parent_->get_stop_bits());
  }
  if (this->parent_->get_data_bits() != data_bits) {
    ESP_LOGE(TAG, "  Invalid number of data bits: Integration requested %u data bits but you have %u!", data_bits,
             this->parent_->get_data_bits());
  }
  if (this->parent_->get_parity() != parity) {
    ESP_LOGE(TAG, "  Invalid parity: Integration requested parity %s but you have %s!",
             LOG_STR_ARG(parity_to_str(parity)), LOG_STR_ARG(parity_to_str(this->parent_->get_parity())));
  }
}

}  // namespace uart
}  // namespace esphome