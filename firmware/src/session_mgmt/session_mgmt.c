#include "session_mgmt.h"
#include "board_conf.h"
#include "common/manikin_types.h"
#include <usbd_cdc_if.h>
#include <stdint.h>
#include "../sampling/sampling.h"
#include "isotp.h"
#include "SEGGER_RTT.h"
#include "flash.h"

#if BOARD_CONF_USE_SENSOR1
size_t sensor1_sample_id;
#endif
#if BOARD_CONF_USE_SENSOR2
size_t sensor2_sample_id;
#endif
#if BOARD_CONF_USE_SENSOR3
size_t sensor3_sample_id;
#endif
static uint8_t comm_buf[128];
IsoTpLink      comm_link;
static uint8_t command_link_isotp_rx_buf[128];
static uint8_t command_link_isotp_tx_buf[128];

static lwrb_t  comm_link_ring_buf;
static uint8_t comm_link_ring_buf_data[1024];

typedef enum
{
    SYSTEM_CMD_STOP,
    SYSTEM_CMD_START,
    SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_1,
    SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_2,
    SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_3,
    SYSTEM_CMD_GET_STATUS,
    SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_1,
    SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_2,
    SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_3,
} system_cmd_t;

typedef enum
{
    SYSTEM_STATE_INIT,
    SYSTEM_STATE_SAMPLING,
    SYSTEM_STATE_ERROR
} system_state_t;

typedef enum
{
    SENSOR_STATUS_OK,
    SENSOR_STATUS_ERROR,
    SENSOR_STATUS_DISCONNECTED,
    SENSOR_STATUS_TIMEOUT
} sensor_health_t;

typedef struct
{
    // Byte 0
    uint8_t id : 4;
    uint8_t startup_ok : 1;
    uint8_t flash_ok : 1;
    uint8_t state : 2;

    // Byte 1
    uint8_t sensor1_sr : 7;
    uint8_t sensor1_health : 2;

    // Byte 2
    uint8_t sensor1_faultcnt : 3;
    uint8_t sensor2_sr : 7;

    // Byte 3
    uint8_t sensor2_health : 2;
    uint8_t sensor2_faultcnt : 3;
    char    sensor1_name[8];
    char    sensor2_name[8];
} system_status_t;

static system_status_t system_status;
static char            latest_log_buffer[2048];

manikin_status_t
session_mgmt_init ()
{
    size_t len = spi_flash_get_log_history(sizeof(latest_log_buffer), latest_log_buffer);
    if (len > 0)
    {
        printf("LEN: %d, Data: %s", len, latest_log_buffer);
        spi_flash_clear_log();
    }
    lwrb_init(&comm_link_ring_buf, comm_link_ring_buf_data, sizeof(comm_link_ring_buf_data));
    lwrb_write(&comm_link_ring_buf, latest_log_buffer, len);
    system_status.id               = BOARD_CONF_SENSORHUB_ID;
    system_status.state            = SYSTEM_STATE_INIT;
    system_status.flash_ok         = 1;
    system_status.startup_ok       = 1;
    system_status.sensor1_faultcnt = 0;
    system_status.sensor1_sr       = BOARD_CONF_SENSOR1_SAMPLE_RATE_HZ;
    system_status.sensor1_health   = SENSOR_STATUS_OK;
    system_status.sensor2_faultcnt = 0;
#if BOARD_CONF_USE_SENSOR2
    system_status.sensor2_sr     = BOARD_CONF_SENSOR2_SAMPLE_RATE_HZ;
    system_status.sensor2_health = SENSOR_STATUS_OK;
#endif
    memcpy((system_status.sensor1_name),
           BOARD_CONF_SENSOR1_NAME,
           sizeof(BOARD_CONF_SENSOR1_NAME) <= 8 ? sizeof(BOARD_CONF_SENSOR1_NAME) : 8);
#if BOARD_CONF_USE_SENSOR2
    memcpy((system_status.sensor2_name),
           BOARD_CONF_SENSOR2_NAME,
           sizeof(BOARD_CONF_SENSOR2_NAME) <= 8 ? sizeof(BOARD_CONF_SENSOR2_NAME) : 8);
#endif
    isotp_init_link(&comm_link,
                    BOARD_CONF_CAN_STATUS_TX_ID,
                    command_link_isotp_tx_buf,
                    sizeof(command_link_isotp_tx_buf),
                    command_link_isotp_rx_buf,
                    sizeof(command_link_isotp_rx_buf));
    return MANIKIN_STATUS_OK;
}

manikin_status_t
session_mgmt_on_global_can_msg (uint8_t *msg, const size_t dlc)
{
    if (dlc == 1)
    {
        if (msg[0] == SYSTEM_CMD_START)
        {
            sensor1_sample_id = 0;
#if BOARD_CONF_USE_SENSOR2
            sensor2_sample_id = 0;
#endif
#if BOARD_CONF_USE_SENSOR3
            sensor3_sample_id = 0;
#endif
            start_sensor_sampling();
            system_status.state = SYSTEM_STATE_SAMPLING;
        }
        else if (msg[0] == 120)
        {
            stop_sensor_sampling();
            system_status.state = SYSTEM_STATE_INIT;
        }
    }
    return MANIKIN_STATUS_OK;
}

manikin_status_t
session_mgmt_on_can_msg (uint8_t *msg, const size_t dlc)
{
    uint8_t transmit_buffer[50];
    switch (msg[0])
    {

        case SYSTEM_CMD_STOP: {
            stop_sensor_sampling();
            break;
        }

        case SYSTEM_CMD_START: {
            start_sensor_sampling();
            break;
        }
        case SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_1: {
            memcpy(transmit_buffer, &sensor1_sample_id, sizeof(size_t));
            isotp_send(&comm_link, transmit_buffer, sizeof(size_t));
            break;
        }
        case SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_2: {
#if BOARD_CONF_USE_SENSOR2
            memcpy(transmit_buffer, &sensor2_sample_id, sizeof(size_t));
            isotp_send(&comm_link, transmit_buffer, sizeof(size_t));
#endif
            break;
        }
        case SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_3: {
#if BOARD_CONF_USE_SENSOR3
            memcpy(transmit_buffer, &sensor3_sample_id, sizeof(size_t));
            isotp_send(&comm_link, transmit_buffer, sizeof(size_t));
#else
            memset(transmit_buffer, 0x00, sizeof(size_t));
            isotp_send(&comm_link, transmit_buffer, sizeof(size_t));
#endif
            break;
        }
        case SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_1: {
            break;
        }
        case SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_2: {
            break;
        }
        case SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_3: {
            break;
        }
        case SYSTEM_CMD_GET_STATUS: {
            memcpy(transmit_buffer, &system_status, sizeof(system_status_t));
            isotp_send(&comm_link, transmit_buffer, sizeof(system_status_t));
            break;
        }
        default: {
            break;
        }
    }
    return MANIKIN_STATUS_OK;
}

manikin_status_t
session_mgmt_on_usb_msg (uint8_t *msg, const size_t size)
{
    uint8_t transmit_buffer[50];
    switch (msg[0])
    {

        case SYSTEM_CMD_STOP: {
            stop_sensor_sampling();
            break;
        }

        case SYSTEM_CMD_START: {
            start_sensor_sampling();
            break;
        }
        case SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_1: {
            memcpy(transmit_buffer, &sensor1_sample_id, sizeof(size_t));
            CDC_Transmit_FS(transmit_buffer, sizeof(size_t));
            break;
        }
        case SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_2: {
#if BOARD_CONF_USE_SENSOR2
            memcpy(transmit_buffer, &sensor2_sample_id, sizeof(size_t));
            // isotp_send(&comm_link, transmit_buffer, sizeof(size_t));
            CDC_Transmit_FS(transmit_buffer, sizeof(size_t));
#endif
            break;
        }
        case SYSTEM_CMD_GET_NUM_SAMPLES_SENSOR_3: {
#if BOARD_CONF_USE_SENSOR3
            memcpy(transmit_buffer, &sensor3_sample_id, sizeof(size_t));
            isotp_send(&comm_link, transmit_buffer, sizeof(size_t));
#else
            memset(transmit_buffer, 0x00, sizeof(size_t));
            CDC_Transmit_FS(transmit_buffer, sizeof(size_t));
#endif
            break;
        }
        case SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_1: {
            break;
        }
        case SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_2: {
            break;
        }
        case SYSTEM_CMD_RETRANSMIT_SAMPLE_SENSOR_3: {
            break;
        }
        case SYSTEM_CMD_GET_STATUS: {
            memcpy(transmit_buffer, &system_status, sizeof(system_status_t));
            CDC_Transmit_FS(transmit_buffer, sizeof(system_status_t));
            break;
        }
        default: {
            break;
        }
    }
    return MANIKIN_STATUS_OK;
}

manikin_status_t
session_mgmt_check_for_can_cmd ()
{
    uint32_t recv_size;
    int      ret = isotp_receive(&comm_link, comm_buf, sizeof(comm_buf), &recv_size);
    if (ISOTP_RET_OK == ret)
    {
        /* Handle received message */
        SEGGER_RTT_printf(0, "Received %d bytes:\n", recv_size);
        SEGGER_RTT_printf(0, "%s\n", comm_buf);
        session_mgmt_on_can_msg(comm_buf, recv_size);
    }
    return MANIKIN_STATUS_OK;
}

manikin_status_t
session_mgmt_deinit ()
{
    return MANIKIN_STATUS_OK;
}