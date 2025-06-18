#ifndef MANIKIN_WATCHDOG_FUNCTIONS_H
#define MANIKIN_WATCHDOG_FUNCTIONS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stddef.h>
#include "manikin_software_conf.h"

    int watchdog_hal_init(manikin_watchdog_inst_t timer_inst, uint32_t timeout);

    int watchdog_hal_kick(manikin_watchdog_inst_t timer_inst);

    int watchdog_hal_deinit(manikin_watchdog_inst_t timer_inst);

#ifdef __cplusplus
}
#endif
#endif /* MANIKIN_WATCHDOG_FUNCTIONS_H */