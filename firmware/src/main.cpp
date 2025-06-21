#include "board_conf.h"
#include "session_mgmt/session_mgmt.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "hal_msp.h"
#include <common/manikin_bit_manipulation.h>
#include <cstdint>
#include "sampling.h"
#include "stm32f4xx_hal.h"
#include "flash.h"
#include "hal_msp.h"
#include "can_wrapper.h"
#include "cli.h"
#include "stm32f4xx_hal_gpio.h"
#include "isotp.h"
#include "SEGGER_RTT.h"

extern lwrb_t              buff;
extern struct sensor_state sensor1;
extern struct sensor_state sensor2;
extern struct sensor_state sensor3;
extern IsoTpLink           comm_link;

int
main (void)
{
    SystemClock_Config();
    HAL_Init();
    BOARD_GPIO_Init();
    SEGGER_RTT_Init();
    manikin_cli_init();
    MX_USB_DEVICE_Init();
    session_mgmt_init();
    init_spi_flash_memory();
    init_peripherals_for_sensors();
    HAL_Delay(1);
    init_can();
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
            manikin_cli_flush(&buff);
            count = 0;
        }
        can_phy_poll();
        __WFI();
    }
}
