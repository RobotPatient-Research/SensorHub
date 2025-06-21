#ifndef SAMPLING_H
#define SAMPLING_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <common/manikin_types.h>
#include <stddef.h>
#include <isotp.h>
#include <sample_timer/sample_timer.h>
#include <lwrb/lwrb.h>

    struct sensor_state
    {
        sample_timer_ctx_t   timer_ctx;
        manikin_sensor_ctx_t sensor_ctx;
        IsoTpLink            iso_tp_link;
        size_t               sample_id;
        lwrb_t               can_ringbuffer;
        uint8_t              can_ringbuffer_data[512];
    };
    /**
     * @brief Initialize I2C, Timer and GPIO peripherals for using the sensors
     */
    manikin_status_t init_peripherals_for_sensors();

    manikin_status_t check_and_sample_sensors();

    manikin_status_t start_sensor_sampling();
    manikin_status_t stop_sensor_sampling();

    manikin_status_t print_to_stdout();

    manikin_status_t print_to_can();
#ifdef __cplusplus
}
#endif
#endif /* SAMPLING_H */