#pragma once

#include <stddef.h>
#include <cstdint>

namespace sdragos
{

    namespace mppsolar
    {
        class BMSLibProtocolDataAdapter
        {
        public:
            // Version information
            virtual uint8_t *getBMSFirmwareVersion() = 0;
            virtual uint8_t *getBMSHardwareVersion() = 0;

            // BMS general status
            virtual uint8_t *getNumberOfCells() = 0;
            virtual uint8_t *getCellVoltageOrNull(size_t cellNumber) = 0;
            virtual uint8_t *getNumberOfTemperatureSensors() = 0;
            virtual uint8_t *getTemperatureOfSensorOrNull(size_t temperatureSensorNumber) = 0;
            virtual uint8_t *getModuleChargeCurrent() = 0;
            virtual uint8_t *getModuleDischargeCurrent() = 0;
            virtual uint8_t *getModuleVoltage() = 0;
            virtual uint8_t *getStateOfCharge() = 0;
            virtual uint8_t *getModuleTotalCapacity() = 0;

            // BMS warning information inquiry
            // All reply with 2 bytes and only the LSB is set to one of:
            //      0x00 - Normal
            //      0x01 - Below normal
            //      0x02 - Above higher limit
            //      0xF0 - Other error
            virtual uint8_t *getNumberOfCellsForWarningInfo() = 0;
            virtual uint8_t *getCellPairVoltageState(size_t oddCellNumber) = 0;
            virtual uint8_t *getNumberOfTemperatureSensorsForWarningInfo() = 0;
            virtual uint8_t *getTemperatureSensorPairState(size_t oddTemperatureSensorNumber) = 0;
            virtual uint8_t *getModuleChargeVoltageState() = 0;
            virtual uint8_t *getModuleDischargeVoltageState() = 0;
            virtual uint8_t *getCellChargeVoltageState() = 0;
            virtual uint8_t *getCellDischargeVoltageState() = 0;
            virtual uint8_t *getModuleChargeCurrentState() = 0;
            virtual uint8_t *getModuleDischargeCurrentState() = 0;
            virtual uint8_t *getModuleChargeTemperatureState() = 0;
            virtual uint8_t *getModuleDischargeTemperatureState() = 0;
            virtual uint8_t *getCellChargeTemperatureState() = 0;
            virtual uint8_t *getCellDischargeTemperatureState() = 0;

            // BMS charge and discharge information inquiry
            virtual uint8_t *getChargeVoltageLimit() = 0;
            virtual uint8_t *getDischargeVoltageLimit() = 0;
            virtual uint8_t *getChargeCurrentLimit() = 0;
            virtual uint8_t *getDischargeCurrentLimit() = 0;
            virtual uint8_t *getChargeDischargeStatus() = 0;
            virtual uint8_t *getRuntimeToEmptySeconds() = 0;
        }; // class BMSLibProtocolDataAdapter
    } // namespace mppsolar
} // namespace sdragos
