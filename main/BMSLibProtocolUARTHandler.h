#pragma once

#include "BMSLibProtocolDataAdapter.h"
#include <map>
#include "uart_idf.h"
#include "uart_device.h"

#define DEVICE_QUERY_FRAME_SIZE 8

using namespace esphome;
using namespace uart;

namespace sdragos
{
    namespace mppsolar
    {

        class BMSLibProtocolUARTHandler : public esphome::uart::UARTDevice{
        public:
            BMSLibProtocolUARTHandler(UARTComponent *parent);
            // This method is required because EspHome gets confused if I try to
            // pass it to the constructor alongside the UARTComponent*. It allows
            // for a delayed setup too, which is fine.
            void setDataAdapter(BMSLibProtocolDataAdapter *dataAdapter);
            void setup();
            void loop();

        private:
            // Hard-coded Slave ID. The implementation will need to be changed if you're planning to use
            // more than 1 BMS on the same bus.
            const uint8_t SLAVE_ID = 0x01;
            const uint8_t COMMAND_READ_DATA = 0x03;
            const uint8_t COMMAND_WRITE_DATA = 0x10;

            // Take note that the implementation of this class will release memory for arrays
            // retrieved by calling methods on the data adapter.
            BMSLibProtocolDataAdapter *_dataAdapter = nullptr;

            bool readLatestIncoming8BytesFrame(uint8_t slaveId);
            void checkAndProcessLatest8BytesFrame();
            void processReadDataFrame(uint8_t *data, size_t len);
            void processWriteDataFrame(uint8_t *pData, size_t len);

            uint16_t calculateModbusCrc16(uint8_t *buffer, size_t len);

            void sendInvalidCrcReply();
            void send2BytesPayloadReply(uint8_t *twoBytes);
            void send4BytesPayloadReply(uint8_t *fourBytes);

            void replyForProtocolType();       // 0x0001
            void replyForProtocolVersion();    // 0x0002
            void replyForBMSFirmwareVersion(); // 0x0003
            void replyForBMSHardwareVersion(); // 0x0004

            void replyForNumberOfCellsRequest();                    // 0x0010
            void replyForCellVoltageRequest(uint16_t dataAddress);  // 0x0N11 - 0x0N24
            void replyForNumberOfTemperatureSensors();              // 0x0025
            void replyForTemperatureRequest(uint16_t dataAddress);  // 0x0N26 - 0x0N2F
            void replyForModuleChargeCurrentRequest();              // 0x0030
            void replyForModuleDischargeCurrentRequest();           // 0x0031
            void replyForModuleVoltageRequest();                    // 0x0032
            void replyForStateOfChargeRequest();                    // 0x0033 required
            void replyForModuleTotalCapacityRequest();              // 0x0034

            void replyForNumberOfCellsWarningInfoRequest();                       // 0x0040
            void replyForCellPairVoltageStateRequest(uint16_t dataAddress);       // 0x0N41 - 0x0N4A
            void replyForNumberOfTemperatureSensorsWarningInfoRequest();          // 0x0050
            void replyForTemperatureSensorPairStateRequest(uint16_t dataAddress); // 0x0N51 - 0x0N55
            void replyForModuleChargeVoltageStateRequest();                       // 0x0060
            void replyForModuleDischargeVoltageStateRequest();                    // 0x0061
            void replyForCellChargeVoltageStateRequest();                         // 0x0062
            void replyForCellDischargeVoltageStateRequest();                      // 0x0063
            void replyForModuleChargeCurrentStateRequest();                       // 0x0064
            void replyForModuleDischargeCurrentStateRequest();                    // 0x0065
            void replyForModuleChargeTemperatureStateRequest();                   // 0x0066
            void replyForModuleDischargeTemperatureStateRequest();                // 0x0067
            void replyForCellChargeTemperatureStateRequest();                     // 0x0068
            void replyForCellDischargeTemperatureStateRequest();                  // 0x0069

            void replyForChargeVoltageLimitRequest();    // 0x0070 required
            void replyForDischargeVoltageLimitRequest(); // 0x0071 required
            void replyForChargeCurrentLimitRequest();    // 0x0072 required
            void replyForDischargeCurrentLimitRequest(); // 0x0073 required
            void replyForChargeDischargeStatusRequest(); // 0x0074 required

            void replyForRuntimeToEmptyRequest(); // 0x0075

            uint8_t _rxBuffer[DEVICE_QUERY_FRAME_SIZE]{};
            size_t _rxBufferIndex = 0;
            bool _frameStarted = false;

            using pReplyToRequestNoParamFunc = void (BMSLibProtocolUARTHandler::*)();
            using pReplyToRequestDataAddressFunc = void (BMSLibProtocolUARTHandler::*)(uint16_t);

            std::map<uint16_t, pReplyToRequestNoParamFunc> _mapDataAddressToReplyToRequestNoParamFunc{};
            std::map<uint16_t, pReplyToRequestDataAddressFunc> _mapDataAddressToReplyToRequestDataAddressFunc{};
        }; // class BMSLibProtocolUARTHandler

    } // namespace mppsolar
} // namespace sdragos