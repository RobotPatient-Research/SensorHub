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
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal_gpio.h"
#include "isotp.h"

IsoTpLink g_link;
IsoTpLink g_link2;
IsoTpLink g_link3;
IsoTpLink comm_link;

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
    uint8_t  read_buf[16];
    uint8_t  read_buf2[16];
    uint8_t  read_buf3[50];
    uint8_t  comm_buf[128];
    uint32_t recv_size;
    uint16_t count = 0;
    while (1)
    {
        int ret
            = isotp_receive(&comm_link, comm_buf, sizeof(comm_buf), &recv_size);
        if (ISOTP_RET_OK == ret)
        {
            /* Handle received message */
            SEGGER_RTT_printf(0, "Received %d bytes:\n", recv_size);
            SEGGER_RTT_printf(0, "%s\n", comm_buf);
            session_mgmt_on_can_msg(comm_buf, recv_size);
        }
        check_and_sample_sensor1(read_buf);
        check_and_sample_sensor2(read_buf2);
        check_and_sample_sensor3(read_buf3);
        if (count < 5)
        {
            count++;
        }
        else
        {
            print_to_can();
            print_to_stdout();
            count = 0;
        }
        isotp_poll(&g_link);
        isotp_poll(&g_link2);
        isotp_poll(&g_link3);
        isotp_poll(&comm_link);
        __WFI();
    }
}
