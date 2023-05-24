set(SOURCES uart_common.cpp uart_component.cpp uart_device.cpp uart_idf.cpp BMSLibProtocolUARTHandler.cpp BMSLibProtocolMockDataAdapter.cpp main.cpp)
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ../include)