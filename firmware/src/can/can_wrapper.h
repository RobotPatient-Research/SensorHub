#ifndef CAN_WRAPPER_H
#define CAN_WRAPPER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"
#include "stddef.h"
    /**
     * @brief Initialize the CAN peripheral with default settings (as defined in
     * board_conf.h)
     */
    void init_can();
    /**
     * @brief Transmit data over CAN as soon as bus comes free
     * @param data Pointer to the write-buffer
     * @param len The number of bytes to write
     */
    size_t can_phy_transmit(uint32_t arb_id, uint8_t *data, size_t len);
#ifdef __cplusplus
}
#endif
#endif // CAN_WRAPPER_H