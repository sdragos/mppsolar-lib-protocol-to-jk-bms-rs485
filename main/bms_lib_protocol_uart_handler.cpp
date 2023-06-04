#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include "bms_lib_protocol_uart_handler.h"

using namespace esphome;
using namespace uart;

namespace sdragos
{
    namespace mppsolar
    {
        BMSLibProtocolUARTHandler::BMSLibProtocolUARTHandler(UARTComponent *parent) : UARTDevice(parent){
            // Setting up functions that will be used to reply to ReadData device queries
            _mapDataAddressToReplyToRequestNoParamFunc[0x0001] = &BMSLibProtocolUARTHandler::replyForProtocolType;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0002] = &BMSLibProtocolUARTHandler::replyForProtocolVersion;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0003] = &BMSLibProtocolUARTHandler::replyForBMSFirmwareVersion;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0005] = &BMSLibProtocolUARTHandler::replyForBMSHardwareVersion;

            _mapDataAddressToReplyToRequestNoParamFunc[0x0010] = &BMSLibProtocolUARTHandler::replyForNumberOfCellsRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0025] = &BMSLibProtocolUARTHandler::replyForNumberOfTemperatureSensors;

            _mapDataAddressToReplyToRequestNoParamFunc[0x0030] = &BMSLibProtocolUARTHandler::replyForModuleChargeCurrentRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0031] = &BMSLibProtocolUARTHandler::replyForModuleDischargeCurrentRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0032] = &BMSLibProtocolUARTHandler::replyForModuleVoltageRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0033] = &BMSLibProtocolUARTHandler::replyForStateOfChargeRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0034] = &BMSLibProtocolUARTHandler::replyForModuleTotalCapacityRequest;

            _mapDataAddressToReplyToRequestNoParamFunc[0x0040] = &BMSLibProtocolUARTHandler::replyForNumberOfCellsWarningInfoRequest;

            _mapDataAddressToReplyToRequestNoParamFunc[0x0050] = &BMSLibProtocolUARTHandler::replyForNumberOfTemperatureSensorsWarningInfoRequest;

            _mapDataAddressToReplyToRequestNoParamFunc[0x0060] = &BMSLibProtocolUARTHandler::replyForModuleChargeVoltageStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0061] = &BMSLibProtocolUARTHandler::replyForModuleDischargeVoltageStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0062] = &BMSLibProtocolUARTHandler::replyForCellChargeVoltageStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0063] = &BMSLibProtocolUARTHandler::replyForCellDischargeVoltageStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0064] = &BMSLibProtocolUARTHandler::replyForModuleChargeCurrentStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0065] = &BMSLibProtocolUARTHandler::replyForModuleDischargeCurrentStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0066] = &BMSLibProtocolUARTHandler::replyForModuleChargeTemperatureStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0067] = &BMSLibProtocolUARTHandler::replyForModuleDischargeTemperatureStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0068] = &BMSLibProtocolUARTHandler::replyForCellChargeTemperatureStateRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0069] = &BMSLibProtocolUARTHandler::replyForCellDischargeTemperatureStateRequest;

            _mapDataAddressToReplyToRequestNoParamFunc[0x0070] = &BMSLibProtocolUARTHandler::replyForChargeVoltageLimitRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0071] = &BMSLibProtocolUARTHandler::replyForDischargeVoltageLimitRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0072] = &BMSLibProtocolUARTHandler::replyForChargeCurrentLimitRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0073] = &BMSLibProtocolUARTHandler::replyForDischargeCurrentLimitRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0074] = &BMSLibProtocolUARTHandler::replyForChargeDischargeStatusRequest;
            _mapDataAddressToReplyToRequestNoParamFunc[0x0075] = &BMSLibProtocolUARTHandler::replyForRuntimeToEmptyRequest;

            for (uint16_t i = 0x0000; i <= 0x000F; i++)
            {
                for (uint16_t j = 0x0011; j <= 0x0024; j++)
                {
                    uint16_t addr = (i << 8) | j;
                    _mapDataAddressToReplyToRequestDataAddressFunc[addr] = &BMSLibProtocolUARTHandler::replyForCellVoltageRequest;
                }
            }

            for (uint16_t i = 0x0000; i <= 0x000F; i++)
            {
                for (uint16_t j = 0x0026; j <= 0x002F; j++)
                {
                    uint16_t addr = (i << 8) | j;
                    _mapDataAddressToReplyToRequestDataAddressFunc[addr] = &BMSLibProtocolUARTHandler::replyForTemperatureRequest;
                }
            }

            for (uint16_t i = 0x0000; i <= 0x000F; i++)
            {
                for (uint16_t j = 0x0041; j <= 0x004A; j++)
                {
                    uint16_t addr = (i << 8) | j;
                    _mapDataAddressToReplyToRequestDataAddressFunc[addr] = &BMSLibProtocolUARTHandler::replyForCellPairVoltageStateRequest;
                }
            }

            for (uint16_t i = 0x0000; i <= 0x000F; i++)
            {
                for (uint16_t j = 0x0051; j <= 0x0055; j++)
                {
                    uint16_t addr = (i << 8) | j;
                    _mapDataAddressToReplyToRequestDataAddressFunc[addr] = &BMSLibProtocolUARTHandler::replyForTemperatureSensorPairStateRequest;
                }
            }
        }

        void BMSLibProtocolUARTHandler::setup()
        {
            ESP_LOGD("BMSLibProtocolUARTHandler", "Test debug message.");
            ESP_LOGE("BMSLibProtocolUARTHandler", "Test error message.");
            ESP_LOGW("BMSLibProtocolUARTHandler", "Test warning message.");
            ESP_LOGI("BMSLibProtocolUARTHandler", "Test information message.");
        }

        void BMSLibProtocolUARTHandler::loop()
        {
            if (readLatestIncoming8BytesFrame(SLAVE_ID))
            {
                checkAndProcessLatest8BytesFrame();

                // this resets the buffer, so that we don't process the same frame multiple times
                _rxBufferIndex = 0;
            }
        }

        void BMSLibProtocolUARTHandler::setDataAdapter(BMSLibProtocolDataAdapter *dataAdapter)
        {
            this->_dataAdapter = dataAdapter;
        }

        bool BMSLibProtocolUARTHandler::readLatestIncoming8BytesFrame(uint8_t slaveId)
        {
            /*
              Reading and replying to frames on time is critical due to the nature of the Lib protocol.

              Imagine what could happen if you send a reply frame containing max charging Amps to a
              request from the inverter that asks for the voltage that you haven't read yet.

              Due to the above it is of paramount importance that we read everything that is in the UART buffer
              and only process when we get a full 8 bytes frame and there're no other incoming bytes to process.

              The code will check again before sending a reply if anything is available in the buffer and
              skip the reply when that's the case.

               :O - let's not blow up the house
            */
            size_t iterationCount = 0;
            while (this->available())
            {
                uint8_t byteRead = read();

                // If buffer is full, look for a new SLAVE_ID that might be the start of a frame.
                // This is practically a sliding window over incoming bytes.
                if (_rxBufferIndex >= DEVICE_QUERY_FRAME_SIZE)
                {
                    size_t i = 1;
                    while (i < DEVICE_QUERY_FRAME_SIZE && _rxBuffer[i] != slaveId)
                        ++i;

                    if (i < DEVICE_QUERY_FRAME_SIZE) // SLAVE_ID found
                    {
                        size_t remainingBytes = DEVICE_QUERY_FRAME_SIZE - i;
                        memmove(_rxBuffer, _rxBuffer + i, remainingBytes);
                        _rxBufferIndex = remainingBytes;
                    }
                    else // SLAVE_ID not found
                    {
                        _rxBufferIndex = 0;
                    }
                }

                iterationCount++;

                // If we're at the start of the buffer, look for SLAVE_ID to start a frame
                if (_rxBufferIndex == 0 && byteRead != slaveId){
                    if (iterationCount > 15){
                        // The iteration has been going on for too long without
                        // a result, so we should break out of it and let other
                        // processes do work too.
                        break;
                    }
                    else{
                        continue;
                    }
                }

                // Add byte to the buffer
                _rxBuffer[_rxBufferIndex++] = byteRead;
            }

            return (_rxBufferIndex == DEVICE_QUERY_FRAME_SIZE);
        }

        uint16_t BMSLibProtocolUARTHandler::calculateModbusCrc16(uint8_t *buffer, size_t len)
        {
            uint16_t crc = 0xFFFF;
            for (size_t i = 0; i < len; ++i)
            {
                crc ^= buffer[i];
                for (int j = 0; j < 8; ++j)
                {
                    if (crc & 0x0001)
                    {
                        crc = (crc >> 1) ^ 0xA001;
                    }
                    else
                    {
                        crc >>= 1;
                    }
                }
            }
            return crc;
        }

        void BMSLibProtocolUARTHandler::checkAndProcessLatest8BytesFrame()
        {
            // This method assumes that we have an 8 bytes frame in the buffer
            // and that it starts with the desired slave identifier.
            uint16_t receivedCrc = (_rxBuffer[7] << 8) | _rxBuffer[6];
            if (calculateModbusCrc16(_rxBuffer, 6) != receivedCrc)
            {
                ESP_LOGE("BMSLibProtocolUARTHandler", "%s", "Invalid CRC.");
                sendInvalidCrcReply();
                return;
            }

            if (_rxBuffer[1] == COMMAND_READ_DATA)
            {
                processReadDataFrame(_rxBuffer, DEVICE_QUERY_FRAME_SIZE);
            }
            else if (_rxBuffer[1] == COMMAND_WRITE_DATA)
            {
                processWriteDataFrame(_rxBuffer, DEVICE_QUERY_FRAME_SIZE);
            }
            else
            {
                ESP_LOGE("BMSLibProtocolUARTHandler", "Unknown command %d", _rxBuffer[1]);
            }
        }

        void BMSLibProtocolUARTHandler::processWriteDataFrame(uint8_t *pData, size_t len)
        {
            // Verify the command length and CRC
            if (len < 8)
            {
                ESP_LOGE("BMSLibProtocolUARTHandler", "%s", "Invalid command length. Skipping frame.");
                return;
            }
            uint16_t receivedCrc = (pData[len - 1] << 8) | pData[len - 2];
            if (calculateModbusCrc16(pData, len - 2) != receivedCrc)
            {
                ESP_LOGE("BMSLibProtocolUARTHandler", "%s", "Invalid CRC.");
                sendInvalidCrcReply();
                return;
            }

            // todo: implement write data here
        }

        void BMSLibProtocolUARTHandler::processReadDataFrame(uint8_t *pData, size_t len)
        {
            const uint16_t dataAddress = (pData[2] << 8) | pData[3];

            // Unused. (Why does this value need to be in the frame when we know the value based on the dataAddress?)
            uint16_t dataLength = (pData[4] << 8) | pData[5];

            auto pFuncNoParam = _mapDataAddressToReplyToRequestNoParamFunc.find(dataAddress);
            if (pFuncNoParam != _mapDataAddressToReplyToRequestNoParamFunc.end())
            {
                // Call the processing function for dataAddress values that do not require
                // the address as parameter
                BMSLibProtocolUARTHandler::pReplyToRequestNoParamFunc funcNoParam = pFuncNoParam->second;
                (this->*(funcNoParam))();
            }
            else
            {
                auto pFuncStartAddressParam = _mapDataAddressToReplyToRequestDataAddressFunc.find(dataAddress);
                if (pFuncStartAddressParam != _mapDataAddressToReplyToRequestDataAddressFunc.end())
                {
                    // Call the processing function for dataAddress values that needs the actual
                    // dataAddress to be passed on as a parameter. Usually these commands are implemented
                    // by a single function and the answer depends on the actual dataAddress supplied.
                    BMSLibProtocolUARTHandler::pReplyToRequestDataAddressFunc funcStartAddressParam = pFuncStartAddressParam->second;
                    (this->*(funcStartAddressParam))(dataAddress);
                }
                else
                {
                    ESP_LOGE("BMSLibProtocolUARTHandler", "%s", "Unsupported address received. Skipping frame.");
                    return; // Do not send a reply for unsupported addresses
                }
            }
        }

        void BMSLibProtocolUARTHandler::sendInvalidCrcReply()
        {
            constexpr size_t replyLen = 5;
            const auto reply = new uint8_t[replyLen];
            reply[0] = SLAVE_ID;
            reply[1] = COMMAND_READ_DATA + 128;
            reply[2] = 0x03; // invalid CRC error code
            uint16_t crc = calculateModbusCrc16(reply, replyLen - 2);
            reply[3] = (uint8_t)crc;
            reply[4] = (uint8_t)(crc >> 8);

            if (!available())
                write_array(reply, replyLen);
            delete[] reply;
        }

        void BMSLibProtocolUARTHandler::send2BytesPayloadReply(uint8_t *twoBytes)
        {
            if (sizeof(twoBytes) != 4)
            {
                ESP_LOGE("BMSLibProtocolUARTHandler", "Got unexpected payload size in send2BytesPayloadReply. Stopping. Size: %d", sizeof(twoBytes));
                return;
            }

            constexpr size_t replyLen = 8;
            const auto reply = new uint8_t[replyLen];

            reply[0] = SLAVE_ID;
            reply[1] = COMMAND_READ_DATA;
            reply[2] = 0; // MSB data size
            reply[3] = 1; // LSB data size

            reply[4] = twoBytes[0]; // MSB data
            reply[5] = twoBytes[1]; // LSB data

            // Calculate and append the CRC
            uint16_t crc = calculateModbusCrc16(reply, replyLen - 2);
            reply[6] = (uint8_t)crc;        // LSB
            reply[7] = (uint8_t)(crc >> 8); // MSB

            // Send the reply only when no new bytes were received, otherwise we're too late.
            // If we continue, we might send a reply for a message unknown yet to this code.
            if (!available())
                write_array(reply, replyLen);

            delete[] reply;
            delete[] twoBytes;
        }

        void BMSLibProtocolUARTHandler::send4BytesPayloadReply(uint8_t *fourBytes)
        {
            if (sizeof(fourBytes) != 4)
            {
                ESP_LOGE("BMSLibProtocolUARTHandler", "Got unexpected payload size in send4BytesPayloadReply. Stopping. Size: %d", sizeof(fourBytes));
                return;
            }

            constexpr size_t replyLen = 10;
            const auto reply = new uint8_t[replyLen];

            reply[0] = SLAVE_ID;
            reply[1] = COMMAND_READ_DATA;
            reply[2] = 0; // MSB data size
            reply[3] = 2; // LSB data size

            reply[4] = fourBytes[0]; // MSB data
            reply[5] = fourBytes[1];
            reply[6] = fourBytes[2];
            reply[7] = fourBytes[3]; // LSB data

            // Calculate and append the CRC
            uint16_t crc = calculateModbusCrc16(reply, replyLen - 2);
            reply[8] = (uint8_t)crc;        // LSB
            reply[9] = (uint8_t)(crc >> 8); // MSB

            // Send the reply only when no new bytes were received, otherwise we're too late.
            // If we continue, we might send a reply for a message unknown yet to this code.
            if (!available())
                write_array(reply, replyLen);

            delete[] reply;
            delete[] fourBytes;
        }

        void BMSLibProtocolUARTHandler::replyForProtocolType()
        { // 0x0001, expected 2 bytes reply
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForProtocolType");
            auto reply = new uint8_t[2];
            reply[0] = 0x00;
            reply[1] = 0x00;
            send2BytesPayloadReply(reply);
        }

        void BMSLibProtocolUARTHandler::replyForProtocolVersion()
        { // 0x0002, expected 2 bytes reply
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForProtocolVersion");
            auto reply = new uint8_t[2];
            reply[0] = 0x00;
            reply[1] = 0x00;
            send2BytesPayloadReply(reply);
        }

        void BMSLibProtocolUARTHandler::replyForBMSFirmwareVersion()
        { // 0x0003, expected 4 bytes reply
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForBMSFirmwareVersion");

            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getBMSFirmwareVersion();
                send4BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForBMSHardwareVersion()
        { // 0x0004, expected 4 bytes reply
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForBMSHardwareVersion");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getBMSHardwareVersion();
                send4BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForNumberOfCellsRequest()
        { // 0x0010, expected 2 bytes reply (integer, count of cells)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForNumberOfCellsRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getNumberOfCells();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForCellVoltageRequest(uint16_t dataAddress)
        { // 0x0011 - 0x0024, expected 2 bytes reply (integer, count of 0.1V)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForCellVoltageRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                uint16_t cellLowOrder = (dataAddress & 0x00FF) - 0x0010;
                uint16_t cellHighOrder = (dataAddress >> 8) & 0x00FF;
                uint16_t cellNumber = (cellHighOrder * 20) + cellLowOrder;

                ESP_LOGD("BMSLibProtocolUARTHandler", "replyForCellVoltageRequest for cell: %d", cellNumber);

                auto result = this->_dataAdapter->getCellVoltageOrNull(cellNumber);
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForNumberOfTemperatureSensors()
        { // 0x0025, expected 2 bytes reply (integer, count of sensors)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForNumberOfTemperatureSensors");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getNumberOfTemperatureSensors();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForTemperatureRequest(uint16_t dataAddress)
        { // 0x0026 - 0x002F -- expected 2 bytes reply (number of 0.1K)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForTemperatureRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                uint16_t tempSensorLowOrder = (dataAddress & 0x00FF) - 0x0025;
                uint16_t tempSensorHighOrder = (dataAddress >> 8) & 0x00FF;
                uint16_t sensorNumber = (tempSensorHighOrder * 10) + tempSensorLowOrder;

                ESP_LOGD("BMSLibProtocolUARTHandler", "replyForTemperatureRequest for sensor: %d", sensorNumber);

                auto result = this->_dataAdapter->getTemperatureOfSensorOrNull(sensorNumber);
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleChargeCurrentRequest()
        { // 0x0030 -- expected 2 bytes reply (number of 0.1A)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleChargeCurrentRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleChargeCurrent();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleDischargeCurrentRequest()
        { // 0x0031 -- expected 2 bytes reply (number of 0.1A)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleDischargeCurrentRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleDischargeCurrent();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleVoltageRequest()
        { // 0x0032 -- expected 2 bytes reply (number of 0.1V)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleVoltageRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleVoltage();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForStateOfChargeRequest()
        { // 0x0033 required -- expected 2 bytes reply (percentage)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForStateOfChargeRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getStateOfCharge();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleTotalCapacityRequest()
        { // 0x0034 -- expected 4 bytes reply (number of mAh)
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleTotalCapacityRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleTotalCapacity();
                send4BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForNumberOfCellsWarningInfoRequest()
        { // 0x0040
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForNumberOfCellsWarningInfoRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getNumberOfCellsForWarningInfo();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForCellPairVoltageStateRequest(uint16_t dataAddress)
        { // 0x0041 - 0x004A
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForCellPairVoltageStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                uint16_t tempSensorLowOrder = ((dataAddress & 0x00FF) - 0x0040) * 2 - 1;
                uint16_t tempSensorHighOrder = (dataAddress >> 8) & 0x00FF;
                uint16_t oddCellNumber = (tempSensorHighOrder * 20) + tempSensorLowOrder;

                ESP_LOGD("BMSLibProtocolUARTHandler", "replyForCellPairVoltageStateRequest for sensor: %d", oddCellNumber);

                auto result = this->_dataAdapter->getCellPairVoltageState(oddCellNumber);
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForNumberOfTemperatureSensorsWarningInfoRequest()
        { // 0x0050
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForNumberOfTemperatureSensorsWarningInfoRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getNumberOfTemperatureSensorsForWarningInfo();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForTemperatureSensorPairStateRequest(uint16_t dataAddress)
        { // 0x0051 - 0x0055
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForTemperatureSensorPairStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                uint16_t tempSensorLowOrder = ((dataAddress & 0x00FF) - 0x0050) * 2 - 1;
                uint16_t tempSensorHighOrder = (dataAddress >> 8) & 0x00FF;
                uint16_t oddSensorNumber = (tempSensorHighOrder * 10) + tempSensorLowOrder;

                ESP_LOGD("BMSLibProtocolUARTHandler", "replyForTemperatureSensorPairStateRequest for sensor: %d", oddSensorNumber);

                auto result = this->_dataAdapter->getTemperatureSensorPairState(oddSensorNumber);
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleChargeVoltageStateRequest()
        { // 0x0060
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleChargeVoltageStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleChargeVoltageState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleDischargeVoltageStateRequest()
        { // 0x0061
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleDischargeVoltageStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleDischargeVoltageState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForCellChargeVoltageStateRequest()
        { // 0x0062
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForCellChargeVoltageStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getCellChargeVoltageState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForCellDischargeVoltageStateRequest()
        { // 0x0063
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForCellDischargeVoltageStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getCellDischargeVoltageState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleChargeCurrentStateRequest()
        { // 0x0064
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleChargeCurrentStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleChargeCurrentState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleDischargeCurrentStateRequest()
        { // 0x0065
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleDischargeCurrentStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleDischargeCurrentState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleChargeTemperatureStateRequest()
        { // 0x0066
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleChargeTemperatureStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleChargeTemperatureState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForModuleDischargeTemperatureStateRequest()
        { // 0x0067
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForModuleDischargeTemperatureStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getModuleDischargeTemperatureState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForCellChargeTemperatureStateRequest()
        { // 0x0068
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForCellChargeTemperatureStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getCellChargeTemperatureState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForCellDischargeTemperatureStateRequest()
        { // 0x0069
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForCellDischargeTemperatureStateRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getCellDischargeTemperatureState();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForChargeVoltageLimitRequest()
        { // 0x0070 required
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForChargeVoltageLimitRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getChargeVoltageLimit();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForDischargeVoltageLimitRequest()
        { // 0x0071 required
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForDischargeVoltageLimitRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getDischargeVoltageLimit();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForChargeCurrentLimitRequest()
        { // 0x0072 required
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForChargeCurrentLimitRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getChargeCurrentLimit();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForDischargeCurrentLimitRequest()
        { // 0x0073 required
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForDischargeCurrentLimitRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getDischargeCurrentLimit();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForChargeDischargeStatusRequest()
        { // 0x0074 required
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForChargeDischargeStatusRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getChargeDischargeStatus();
                send2BytesPayloadReply(result);
            }
        }

        void BMSLibProtocolUARTHandler::replyForRuntimeToEmptyRequest()
        { // 0x0075
            ESP_LOGD("BMSLibProtocolUARTHandler", "%s", "replyForRuntimeToEmptyRequest");
            if (this->_dataAdapter != nullptr && this->_dataAdapter->hasData())
            {
                auto result = this->_dataAdapter->getRuntimeToEmptySeconds();
                send2BytesPayloadReply(result);
            }
        }
    } // namespace mppsolar
} // namespace sdragos