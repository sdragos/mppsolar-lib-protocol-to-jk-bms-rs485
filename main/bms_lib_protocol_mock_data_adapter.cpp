#include "bms_lib_protocol_mock_data_adapter.h"

namespace sdragos
{
    namespace mppsolar
    {
        bool BMSLibProtocolMockDataAdapter::hasData()
        {
            return true;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getBMSFirmwareVersion()
        {
            // returns 4 bytes
            auto reply = new uint8_t[4];
            reply[0] = 0;
            reply[1] = 0;
            reply[2] = 0;
            reply[3] = 1;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getBMSHardwareVersion()
        {
            // returns 4 bytes
            auto reply = new uint8_t[4];
            reply[0] = 0;
            reply[1] = 0;
            reply[2] = 0;
            reply[3] = 1;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getNumberOfCells()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 8; // 8 cells
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getCellVoltageOrNull(size_t cellNumber)
        {
            // 3.327V --> 33V --- bad precision
            // value * 0.1V
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 33;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getNumberOfTemperatureSensors()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 3; // 3 sensors
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getTemperatureOfSensorOrNull(size_t temperatureSensorNumber)
        {
            uint16_t kelvin = 2931; // ((273.15 + 20Celsius)*100)/10
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(kelvin >> 8);
            reply[1] = (uint8_t)kelvin;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleChargeCurrent()
        {
            uint16_t amps = 572; // 57.2A * 10 (returning divisions of 0.1A)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(amps >> 8);
            reply[1] = (uint8_t)amps;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleDischargeCurrent()
        {
            uint16_t amps = 900; // 90A * 10 (returning divisions of 0.1A)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(amps >> 8);
            reply[1] = (uint8_t)amps;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleVoltage()
        {
            uint16_t volts = 264; // 26.4V * 10 (returning divisions of 0.1V)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(volts >> 8);
            reply[1] = (uint8_t)volts;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getStateOfCharge()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0x00;
            reply[1] = 70; // 70% charged
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleTotalCapacity()
        {
            uint32_t milliAmpHours = 280000; // 280Ah * 1000 (returning divisions of 1 mAh)
            auto reply = new uint8_t[4];
            reply[0] = (uint8_t)(milliAmpHours >> 24);
            reply[1] = (uint8_t)(milliAmpHours >> 16);
            reply[2] = (uint8_t)(milliAmpHours >> 8);
            reply[3] = (uint8_t)milliAmpHours;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getChargeVoltageLimit()
        {
            uint16_t amps = 292; // 29.2V * 10 (returning divisions of 0.1V)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(amps >> 8);
            reply[1] = (uint8_t)amps;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getDischargeVoltageLimit()
        {
            uint16_t amps = 200; // 20V * 10 (returning divisions of 0.1V)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(amps >> 8);
            reply[1] = (uint8_t)amps;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getChargeCurrentLimit()
        {
            uint16_t amps = 1400; // 140A * 10 (returning divisions of 0.1A)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(amps >> 8);
            reply[1] = (uint8_t)amps;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getDischargeCurrentLimit()
        {
            uint16_t amps = 3400; // 340A * 10 (returning divisions of 0.1A)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(amps >> 8);
            reply[1] = (uint8_t)amps;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getChargeDischargeStatus()
        {                                          // returning 2 bytes, the LSB will be created by mixing flags
            const uint8_t fullChargeRequest = 8;   // 0000 1000 Set when BMS needs battery fully charged
            const uint8_t chargeImmediately2 = 16; // 0001 0000 Set when SoC is low, like 10~14%
            const uint8_t chargeImmediately = 32;  // 0010 0000 Set when SoC is very low, like 5~9%
            const uint8_t dischargeEnable = 64;    // 0100 0000
            const uint8_t chargeEnable = 128;      // 1000 0000

            auto reply = new uint8_t[2];
            reply[0] = 0x00;
            reply[1] = chargeEnable | dischargeEnable;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getRuntimeToEmptySeconds()
        {
            uint16_t seconds = 1200; // 20 hours * 60 (returning Seconds)
            auto reply = new uint8_t[2];
            reply[0] = (uint8_t)(seconds >> 8);
            reply[1] = (uint8_t)seconds;
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getNumberOfCellsForWarningInfo()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 8; // 8 cells
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getCellPairVoltageState(size_t oddCellNumber)
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal state
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getNumberOfTemperatureSensorsForWarningInfo()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 4; // 4 sensors
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getTemperatureSensorPairState(size_t oddTemperatureSensorNumber)
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x01; // below normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleChargeVoltageState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleDischargeVoltageState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getCellChargeVoltageState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getCellDischargeVoltageState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0xF0; // Other error
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleChargeCurrentState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleDischargeCurrentState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x02; // Above higher limit
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleChargeTemperatureState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getModuleDischargeTemperatureState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getCellChargeTemperatureState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x00; // Normal
            return reply;
        }

        uint8_t * BMSLibProtocolMockDataAdapter::getCellDischargeTemperatureState()
        {
            auto reply = new uint8_t[2];
            reply[0] = 0;
            reply[1] = 0x02; // Above higher limit
            return reply;
        }
    } // namespace mppsolar
} // namespace sdragos