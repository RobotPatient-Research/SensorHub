#ifndef SESSION_MGMT_H
#define SESSION_MGMT_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "common/manikin_types.h"

    manikin_status_t session_mgmt_init();

    manikin_status_t session_mgmt_on_global_can_msg(uint8_t     *msg,
                                                    const size_t dlc);

    manikin_status_t session_mgmt_on_can_msg(uint8_t *msg, const size_t dlc);

    manikin_status_t session_mgmt_on_usb_msg(uint8_t *msg, const size_t size);

    manikin_status_t session_mgmt_deinit();

#ifdef __cplusplus
}
#endif
#endif /* SESSION_MGMT_H */