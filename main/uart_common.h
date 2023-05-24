#pragma once

#include <array>
#include <cstring>
#include <optional>
#include <vector>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hal/cpu_hal.h>
#include <cmath>
#include "esphome_log.h"

namespace esphome{
  namespace uart{
    enum UARTParityOptions
    {
      UART_CONFIG_PARITY_NONE,
      UART_CONFIG_PARITY_EVEN,
      UART_CONFIG_PARITY_ODD,
    };

    const LogString *parity_to_str(UARTParityOptions parity);
  } // namespace uart
} // namespace esphome