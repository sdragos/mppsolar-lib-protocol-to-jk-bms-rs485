set(SOURCES 
    ../include/esphome/core/component.cpp
    ../include/esphome/components/uart/uart.cpp
    ../include/esphome/components/uart/uart_component.cpp
    ../include/esphome/components/uart/uart_component_esp_idf.cpp
    ../include/esphome/components/jk_modbus/jk_modbus.cpp
    bms_lib_protocol_uart_handler.cpp
    bms_lib_protocol_mock_data_adapter.cpp
    main.cpp)
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ../include)