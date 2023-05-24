#include "uart_common.h"

namespace esphome
{
    namespace uart
    {
        const LogString *parity_to_str(UARTParityOptions parity)
        {
            switch (parity)
            {
            case UART_CONFIG_PARITY_NONE:
                return LOG_STR("NONE");
            case UART_CONFIG_PARITY_EVEN:
                return LOG_STR("EVEN");
            case UART_CONFIG_PARITY_ODD:
                return LOG_STR("ODD");
            default:
                return LOG_STR("UNKNOWN");
            }
        }
    } // namespace uart
} // namespace esphome