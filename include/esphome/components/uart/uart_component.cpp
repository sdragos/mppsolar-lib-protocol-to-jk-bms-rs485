//This file contains code taken from https://github.com/esphome/esphome/blob/dev/esphome/components/uart/uart_component.cpp
//It is likely not a 1 to 1 copy as some things may have been modified so that it works without other implementations from ESPHome

#include "uart_component.h"

namespace esphome
{
  namespace uart
  {

    static const char *const TAG = "UARTComponent";

    const LogString *parity_to_str(UARTParityOptions parity)
    {
        switch (parity)
        {
        case UART_CONFIG_PARITY_NONE:
            return LOG_STR("NONE");
        case UART_CONFIG_PARITY_EVEN:
            return LOG_STR("EVEN");
        case UART_CONFIG_PARITY_ODD:
            return LOG_STR("ODD");
        default:
            return LOG_STR("UNKNOWN");
        }
    }

    bool UARTComponent::check_read_timeout_(size_t len)
    {
      if (this->available() >= int(len))
        return true;

      uint32_t start_time = (uint32_t)(esp_timer_get_time() / 1000ULL);
      while (this->available() < int(len))
      {
        if (((uint32_t)(esp_timer_get_time() / 1000ULL)) - start_time > 100)
        {
          ESP_LOGE(TAG, "Reading from UART timed out at byte %u!", this->available());
          return false;
        }
        vPortYield();
      }
      return true;
    }
  } // namespace uart
} // namespace esphome