//This file contains code taken from https://github.com/esphome/esphome/blob/dev/esphome/components/uart/uart_component_esp_idf.cpp
//It is likely not a 1 to 1 copy as some things may have been modified so that it works without other implementations from ESPHome
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include "uart_component_esp_idf.h"
#include "freertos/semphr.h"

namespace esphome
{

  namespace uart
  {
    static const char *const TAG = "IDFUARTComponent";

    uart_config_t IDFUARTComponent::get_config_()
    {
      uart_parity_t parity = UART_PARITY_DISABLE;
      if (this->parity_ == UART_CONFIG_PARITY_EVEN)
      {
        parity = UART_PARITY_EVEN;
      }
      else if (this->parity_ == UART_CONFIG_PARITY_ODD)
      {
        parity = UART_PARITY_ODD;
      }

      uart_word_length_t data_bits;
      switch (this->data_bits_)
      {
      case 5:
        data_bits = UART_DATA_5_BITS;
        break;
      case 6:
        data_bits = UART_DATA_6_BITS;
        break;
      case 7:
        data_bits = UART_DATA_7_BITS;
        break;
      case 8:
        data_bits = UART_DATA_8_BITS;
        break;
      default:
        data_bits = UART_DATA_BITS_MAX;
        break;
      }

      uart_config_t uart_config;
      uart_config.baud_rate = this->baud_rate_;
      uart_config.data_bits = data_bits;
      uart_config.parity = parity;
      uart_config.stop_bits = this->stop_bits_ == 1 ? UART_STOP_BITS_1 : UART_STOP_BITS_2;
      uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
      uart_config.source_clk = UART_SCLK_APB;
      uart_config.rx_flow_ctrl_thresh = 122;

      return uart_config;
    }

    void IDFUARTComponent::setup()
    {
      if (uarts_in_use_[this->uart_num_]){
        ESP_LOGW(TAG, "UART %d number already in use.", this->uart_num_);
        this->mark_failed();
        return;        
      }

      ESP_LOGI(TAG, "Setting up UART %u...", this->uart_num_);

      this->lock_ = xSemaphoreCreateMutex();

      xSemaphoreTake(this->lock_, portMAX_DELAY);

      uart_config_t uart_config = this->get_config_();
      esp_err_t err = uart_param_config(this->uart_num_, &uart_config);
      if (err != ESP_OK)
      {
        ESP_LOGW(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        this->mark_failed();
        return;
      }

      err = uart_driver_install(this->uart_num_, /* UART RX ring buffer size. */ this->rx_buffer_size_,
                                /* UART TX ring buffer size. If set to zero, driver will not use TX buffer, TX function will
                                   block task until all data have been sent out.*/
                                this->tx_buffer_size_,
                                /* UART event queue size/depth. */
                                this->event_queue_size_, &(this->uart_event_queue_),
                                /* Flags used to allocate the interrupt. */ 0);
      if (err != ESP_OK)
      {
        ESP_LOGW(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        this->mark_failed();
        return;
      }

      int8_t tx = this->tx_pin_ != nullptr ? this->tx_pin_->get_pin() : -1;
      int8_t rx = this->rx_pin_ != nullptr ? this->rx_pin_->get_pin() : -1;

      err = uart_set_pin(this->uart_num_, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
      if (err != ESP_OK)
      {
        ESP_LOGW(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        this->mark_failed();
        return;
      }

      uint32_t invert = 0;
      if (this->tx_pin_ != nullptr && this->tx_pin_->is_inverted())
        invert |= UART_SIGNAL_TXD_INV;
      if (this->rx_pin_ != nullptr && this->rx_pin_->is_inverted())
        invert |= UART_SIGNAL_RXD_INV;

      err = uart_set_line_inverse(this->uart_num_, invert);
      if (err != ESP_OK)
      {
        ESP_LOGW(TAG, "uart_set_line_inverse failed: %s", esp_err_to_name(err));
        this->mark_failed();
        return;
      }

      xSemaphoreGive(this->lock_);
    }

    void IDFUARTComponent::dump_config()
    {
      ESP_LOGD(TAG, "UART Bus:");
      ESP_LOGD(TAG, "  Number: %d", this->uart_num_);
      ESP_LOGD(TAG, "  TX Pin: %u", tx_pin_->get_pin());
      ESP_LOGD(TAG, "  RX Pin: %u", rx_pin_->get_pin());
      if (this->rx_pin_ != nullptr)
      {
        ESP_LOGD(TAG, "  RX Buffer Size: %u", this->rx_buffer_size_);
      }
      ESP_LOGD(TAG, "  Baud Rate: %lu baud", this->baud_rate_);
      ESP_LOGD(TAG, "  Data Bits: %u", this->data_bits_);
      ESP_LOGD(TAG, "  Parity: %d", this->parity_);
      ESP_LOGD(TAG, "  Stop bits: %u", this->stop_bits_);
    }

    void IDFUARTComponent::write_array(const uint8_t *data, size_t len)
    {
      xSemaphoreTake(this->lock_, portMAX_DELAY);
      uart_write_bytes(this->uart_num_, data, len);
      xSemaphoreGive(this->lock_);
    }

    bool IDFUARTComponent::peek_byte(uint8_t *data)
    {
      if (!this->check_read_timeout_())
        return false;
      xSemaphoreTake(this->lock_, portMAX_DELAY);
      if (this->has_peek_)
      {
        *data = this->peek_byte_;
      }
      else
      {
        int len = uart_read_bytes(this->uart_num_, data, 1, 20 / portTICK_PERIOD_MS);
        if (len == 0)
        {
          *data = 0;
        }
        else
        {
          this->has_peek_ = true;
          this->peek_byte_ = *data;
        }
      }
      xSemaphoreGive(this->lock_);
      return true;
    }

    bool IDFUARTComponent::read_array(uint8_t *data, size_t len)
    {
      size_t length_to_read = len;
      if (!this->check_read_timeout_(len))
        return false;
      xSemaphoreTake(this->lock_, portMAX_DELAY);
      if (this->has_peek_)
      {
        length_to_read--;
        *data = this->peek_byte_;
        data++;
        this->has_peek_ = false;
      }
      if (length_to_read > 0)
        uart_read_bytes(this->uart_num_, data, length_to_read, 20 / portTICK_PERIOD_MS);
      xSemaphoreGive(this->lock_);

      return true;
    }

    int IDFUARTComponent::available()
    {
      size_t available;

      xSemaphoreTake(this->lock_, portMAX_DELAY);
      uart_get_buffered_data_len(this->uart_num_, &available);
      if (this->has_peek_)
        available++;
      xSemaphoreGive(this->lock_);

      return available;
    }

    void IDFUARTComponent::flush()
    {
      ESP_LOGW(TAG, "    Flushing...");
      xSemaphoreTake(this->lock_, portMAX_DELAY);
      uart_wait_tx_done(this->uart_num_, portMAX_DELAY);
      xSemaphoreGive(this->lock_);
    }
  } // namespace uart
} // namespace esphome