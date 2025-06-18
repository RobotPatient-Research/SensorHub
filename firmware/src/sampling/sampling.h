#ifndef SAMPLING_H
#define SAMPLING_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <common/manikin_types.h>
#include <stddef.h>

    /**
     * @brief Initialize I2C, Timer and GPIO peripherals for using the sensors
     */
    manikin_status_t init_peripherals_for_sensors();

    /**
     * @brief Checks and sample when sensor 1 has new measurement ready (poll
     * timer flag)
     * @param data_buf The buffer to store the resulting samples
     */
    manikin_status_t check_and_sample_sensor1(uint8_t *data_buf);

    /**
     * @brief Checks and sample when sensor 2 has new measurement ready (poll
     * timer flag)
     * @param data_buf The buffer to store the resulting samples
     */
    manikin_status_t check_and_sample_sensor2(uint8_t *data_buf);

    /**
     * @brief Checks and sample when sensor 3 has new measurement ready (poll
     * timer flag)
     * @param data_buf The buffer to store the resulting samples
     */
    manikin_status_t check_and_sample_sensor3(uint8_t *data_buf);

    manikin_status_t start_sensor_sampling();
    manikin_status_t stop_sensor_sampling();

    manikin_status_t print_to_stdout();

    manikin_status_t print_to_can();
#ifdef __cplusplus
}
#endif
#endif /* SAMPLING_H */