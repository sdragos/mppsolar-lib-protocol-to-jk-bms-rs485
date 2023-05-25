#pragma once

#include <stddef.h>
#include <cstdint>

namespace sdragos
{

    namespace mppsolar
    {
        class BMSLibProtocolDataAdapter
        {
        public:                                                                                 // Payload size         Units
            // Returns true when data is available to be read
            virtual bool hasData() = 0;
            // Version information
            virtual uint8_t *getBMSFirmwareVersion() = 0;                                       //      4 bytes
            virtual uint8_t *getBMSHardwareVersion() = 0;                                       //      4 bytes

            // BMS general status
            virtual uint8_t *getNumberOfCells() = 0;                                            //      2 bytes         1 count
            virtual uint8_t *getCellVoltageOrNull(size_t cellNumber) = 0;                       //      2 bytes         0.1V
            virtual uint8_t *getNumberOfTemperatureSensors() = 0;                               //      2 bytes         1 count
            virtual uint8_t *getTemperatureOfSensorOrNull(size_t temperatureSensorNumber) = 0;  //      2 bytes         0.1K
            virtual uint8_t *getModuleChargeCurrent() = 0;                                      //      2 bytes         0.1A
            virtual uint8_t *getModuleDischargeCurrent() = 0;                                   //      2 bytes         0.1A
            virtual uint8_t *getModuleVoltage() = 0;                                            //      2 bytes         0.1V
            virtual uint8_t *getStateOfCharge() = 0;                                            //      2 bytes         1%
            virtual uint8_t *getModuleTotalCapacity() = 0;                                      //      4 bytes         1mAh

            // BMS warning information inquiry
            // All reply with 2 bytes and only the LSB is set to one of:
            //      0x00 - Normal
            //      0x01 - Below normal
            //      0x02 - Above higher limit
            //      0xF0 - Other error
            virtual uint8_t *getNumberOfCellsForWarningInfo() = 0;                              //      2 bytes         1 count
            virtual uint8_t *getCellPairVoltageState(size_t oddCellNumber) = 0;
            virtual uint8_t *getNumberOfTemperatureSensorsForWarningInfo() = 0;                 //      2 bytes         1 count
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
            virtual uint8_t *getChargeVoltageLimit() = 0;                                       //      2 bytes         0.1V
            virtual uint8_t *getDischargeVoltageLimit() = 0;                                    //      2 bytes         0.1V
            virtual uint8_t *getChargeCurrentLimit() = 0;                                       //      2 bytes         0.1A
            virtual uint8_t *getDischargeCurrentLimit() = 0;                                    //      2 bytes         0.1A
            
            // returning 2 bytes, the LSB will be created by mixing flags
            // const uint8_t fullChargeRequest = 8;   // 0000 1000 Set when BMS needs battery fully charged
            // const uint8_t chargeImmediately2 = 16; // 0001 0000 Set when SoC is low, like 10~14%
            // const uint8_t chargeImmediately = 32;  // 0010 0000 Set when SoC is very low, like 5~9%
            // const uint8_t dischargeEnable = 64;    // 0100 0000
            // const uint8_t chargeEnable = 128;      // 1000 0000
            virtual uint8_t *getChargeDischargeStatus() = 0;                                    //      2 bytes         N/A
            virtual uint8_t *getRuntimeToEmptySeconds() = 0;                                    //      2 bytes         1s
        }; // class BMSLibProtocolDataAdapter
    } // namespace mppsolar
} // namespace sdragos
