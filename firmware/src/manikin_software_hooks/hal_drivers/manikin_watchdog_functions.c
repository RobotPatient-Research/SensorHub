#include <stdint.h>
#include "manikin_watchdog_functions.h"
#include "stm32f4xx_ll_wwdg.h"
#include "stm32f4xx_hal.h"

int
watchdog_hal_init (manikin_watchdog_inst_t timer_inst, uint32_t timeout)
{
    return 0;
}

int
watchdog_hal_kick (manikin_watchdog_inst_t timer_inst)
{
    return 0;
}

int
watchdog_hal_deinit (manikin_watchdog_inst_t timer_inst)
{
    return 0;
}
