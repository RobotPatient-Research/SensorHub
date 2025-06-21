#ifndef SESSION_MGMT_H
#define SESSION_MGMT_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "isotp.h"
#include "common/manikin_types.h"

    /**
     * @brief Handle to internally initialized communication-link for unicast CAN messages to/from master node.
     */
    extern IsoTpLink comm_link;

    /**
     * @brief This initializes the session management module, responsible for starting/stopping sampling
     *        and handling status requests.
     * @return MANIKIN_STATUS_OK on successful initialization
     */
    manikin_status_t session_mgmt_init();

    /**
     * @brief Message handler for broadcast messages on CAN-bus (Messages send to all nodes, start/stop e.g.)
     * @param msg Pointer to the byte-buffer containing the message payload
     * @param dlc Length in bytes of the message payload
     * @return MANIKIN_STATUS_OK on successful parse.
     */
    manikin_status_t session_mgmt_on_global_can_msg(uint8_t *msg, size_t dlc);

    /**
     * @brief Message handler for incoming iso-tp messages on command-link CAN-bus
     *        (Messages from master-node directly send to this CAN-node).
     * @param msg Pointer to the byte-buffer containing the message payload
     * @param dlc Length in bytes of the message payload
     * @return MANIKIN_STATUS_OK on successful parse.
     */
    manikin_status_t session_mgmt_on_can_msg(uint8_t *msg, size_t dlc);

    /**
     * @brief Message handler for incoming command messages on USB-CDC (serial).
     * @param msg Pointer to the byte-buffer containing the message payload
     * @param size Length in bytes of the message payload
     * @return MANIKIN_STATUS_OK on successful parse.
     */
    manikin_status_t session_mgmt_on_usb_msg(uint8_t *msg, size_t size);

    /**
     * @brief Poll function for checking the iso-tp command link messages which were put in internal fifo.
     * @return MANIKIN_STATUS_OK on success.
     */
    manikin_status_t session_mgmt_check_for_can_cmd();

    /**
     * @brief Deinitialize the session-management module
     * @return MANIKIN_STATUS_OK on success.
     */
    manikin_status_t session_mgmt_deinit();

#ifdef __cplusplus
}
#endif
#endif /* SESSION_MGMT_H */