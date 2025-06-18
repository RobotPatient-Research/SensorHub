#ifndef CLI_H
#define CLI_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"
#include "stddef.h"

    void cli_on_input(uint8_t *buf, uint32_t len);

    /**
     * @brief This function get's called when a new sample has been read from
     * the sensors It queues the sample for transmit over USB
     */
    size_t manikin_cli_on_new_sensor_sample(uint8_t       *cbor_buf,
                                            uint16_t       cbor_buf_size,
                                            const char    *sample_name,
                                            const size_t   sample_id,
                                            const uint8_t *buffer,
                                            size_t         len);

    void manikin_cli_init();

#ifdef __cplusplus
}
#endif
#endif /* CLI_H */