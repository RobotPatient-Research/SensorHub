#include "board_conf.h"
#include "session_mgmt/session_mgmt.h"
#include "usb_device.h"
#include "hal_msp.h"
#include <stdint.h>
#include "sampling.h"
#include "stm32f4xx_hal.h"
#include "flash.h"
#include "can_wrapper.h"
#include "cli.h"
#include "SEGGER_RTT.h"


int
main ()
{
    SystemClock_Config();
    HAL_Init();
    BOARD_GPIO_Init();
    SEGGER_RTT_Init();
    manikin_cli_init();
    MX_USB_DEVICE_Init();
    init_spi_flash_memory();
    init_can();
    session_mgmt_init();
    init_peripherals_for_sensors();

    uint16_t count = 0;
    while (1)
    {
        session_mgmt_check_for_can_cmd();
        check_and_sample_sensors();
        if (count < 5)
        {
            count++;
        }
        else
        {
            print_to_can();
            manikin_cli_flush();
            spi_flash_flush();
            count = 0;
        }
        can_phy_poll();
        __WFI();
    }
}
