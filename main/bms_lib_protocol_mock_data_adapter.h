#pragma once

#include "bms_lib_protocol_data_adapter.h"

namespace sdragos
{
    namespace mppsolar
    {
        /// @brief Provides a simple mock implementation of the BMSLibProtocolDataAdapter class, that is meant to be
        ///        for understanding and testing of BMSLibProtocol implementations
        class BMSLibProtocolMockDataAdapter : public BMSLibProtocolDataAdapter
        {
            public:
                BMSLibProtocolMockDataAdapter() = default;

                // Version information
                uint8_t *getBMSFirmwareVersion() override;
                uint8_t *getBMSHardwareVersion() override;

                // BMS general status
                uint8_t *getNumberOfCells() override;
                uint8_t *getCellVoltageOrNull(size_t cellNumber) override;
                uint8_t *getNumberOfTemperatureSensors() override;
                uint8_t *getTemperatureOfSensorOrNull(size_t temperatureSensorNumber) override;
                uint8_t *getModuleChargeCurrent() override;
                uint8_t *getModuleDischargeCurrent() override;
                uint8_t *getModuleVoltage() override;
                uint8_t *getStateOfCharge() override;
                uint8_t *getModuleTotalCapacity() override;

                // BMS warning information inquiry
                // All reply with 2 bytes and only the LSB is set to one of:
                //      0x00 - Normal
                //      0x01 - Below normal
                //      0x02 - Above higher limit
                //      0xF0 - Other error
                uint8_t *getNumberOfCellsForWarningInfo() override;
                uint8_t *getCellPairVoltageState(size_t oddCellNumber) override;
                uint8_t *getNumberOfTemperatureSensorsForWarningInfo() override;
                uint8_t *getTemperatureSensorPairState(size_t oddTemperatureSensorNumber) override;
                uint8_t *getModuleChargeVoltageState() override;
                uint8_t *getModuleDischargeVoltageState() override;
                uint8_t *getCellChargeVoltageState() override;
                uint8_t *getCellDischargeVoltageState() override;
                uint8_t *getModuleChargeCurrentState() override;
                uint8_t *getModuleDischargeCurrentState() override;
                uint8_t *getModuleChargeTemperatureState() override;
                uint8_t *getModuleDischargeTemperatureState() override;
                uint8_t *getCellChargeTemperatureState() override;
                uint8_t *getCellDischargeTemperatureState() override;

                // BMS charge and discharge information inquiry
                uint8_t *getChargeVoltageLimit() override;
                uint8_t *getDischargeVoltageLimit() override;
                uint8_t *getChargeCurrentLimit() override;
                uint8_t *getDischargeCurrentLimit() override;
                uint8_t *getChargeDischargeStatus() override;
                uint8_t *getRuntimeToEmptySeconds() override;
        };
    }// namespace mppsolar
}// namespace sdragos