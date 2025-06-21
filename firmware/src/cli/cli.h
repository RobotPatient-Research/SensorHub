#ifndef CLI_H
#define CLI_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <common/manikin_types.h>
#include "stdint.h"
#include "stddef.h"

    /**
     * @brief This function initializes the manikin cli module.
     *        Call this function early in main startup...
     */
    void manikin_cli_init();

    /**
     * @brief This function gets called when new char or input arrives at cli
     *        interface (USB/UART). It saves the response to internal ringbuffer
     * @param buf Pointer to the buffer containing captured input/char
     * @param len Length of the captured input/char in bytes
     */
    void manikin_cli_on_input(const uint8_t *buf, uint32_t len);

    /**
     * @brief This function gets called when a new sample has been read from
     * the sensors It queues the sample for transmit over USB.
     * @param sample_name Name of the sensor which has been sampled
     * @param sample_id   ID of the sample which is being processed
     * @param buffer      Pointer to the buffer containing the sample-data
     * @param len         Length of the sample-data payload in bytes
     * @return Number of bytes encoded (cbor)
     */
    size_t manikin_cli_on_new_sensor_sample(const char    *sample_name,
                                            size_t         sample_id,
                                            const uint8_t *buffer,
                                            size_t         len);

    /**
     * @brief This functions flushes the write-buffer to stdout on call.
     * @return MANIKIN_STATUS_OK on Success
     */
    manikin_status_t manikin_cli_flush();

#ifdef __cplusplus
}
#endif
#endif /* CLI_H */