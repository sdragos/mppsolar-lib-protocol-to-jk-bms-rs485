//Includes for the uart component go here, and some other implementations that I did not bother to look up and
//copy in the proper folder.

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
#include <esphome/core/log.h>

namespace esphome{
  namespace uart{
    class InternalGPIOPin
    {
    public:
      InternalGPIOPin(uint8_t pin, bool is_inverted) : _pin(pin), _is_inverted(is_inverted) {}

      uint8_t get_pin() const { return this->_pin; }
      bool is_inverted() const { return this->_is_inverted; }

    private:
      uint8_t _pin;
      bool _is_inverted;
    };
  } // namespace uart
} // namespace esphome