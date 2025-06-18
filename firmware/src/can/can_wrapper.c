#include "can_wrapper.h"
#include <stdio.h>
#include <stdarg.h>
#include "SEGGER_RTT.h"
#include "board_conf.h"
#include "common/manikin_types.h"

#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_gpio_ex.h"

#include "isotp.h"
#include "../session_mgmt/session_mgmt.h"

extern IsoTpLink g_link;
extern IsoTpLink g_link2;
extern IsoTpLink g_link3;
extern IsoTpLink comm_link;

/* Alloc send and receive buffer statically in RAM */
static uint8_t g_isotpRecvBuf[128];
static uint8_t g_isotpSendBuf[128];

/* Alloc send and receive buffer statically in RAM */
static uint8_t g_isotpRecvBuf2[128];
static uint8_t g_isotpSendBuf2[128];


/* Alloc send and receive buffer statically in RAM */
uint8_t g_isotpRecvBuf3[128];
uint8_t g_isotpSendBuf3[128];

/* Alloc send and receive buffer statically in RAM */
uint8_t g_isotpRecvBuf4[128];
uint8_t g_isotpSendBuf4[128];

/* Static CAN handle structures */
CAN_HandleTypeDef          can_handle;
static CAN_FilterTypeDef   can_filter;
static CAN_RxHeaderTypeDef rx_header;
static uint32_t            tx_mailbox;

/* Initialize CAN peripheral */
int
can_phy_hal_init_can_mcu (const uint32_t speed)
{
    (void)speed; /* Speed is not yet used */

    can_handle.Instance = BOARD_CONF_CAN_INSTANCE;

    can_handle.Init.Mode                 = CAN_MODE_NORMAL;
    can_handle.Init.Prescaler            = 6;
    can_handle.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    can_handle.Init.TimeSeg1             = CAN_BS1_11TQ;
    can_handle.Init.TimeSeg2             = CAN_BS2_2TQ;
    can_handle.Init.TimeTriggeredMode    = DISABLE;
    can_handle.Init.AutoBusOff           = DISABLE;
    can_handle.Init.AutoWakeUp           = DISABLE;
    can_handle.Init.AutoRetransmission   = DISABLE;
    can_handle.Init.ReceiveFifoLocked    = DISABLE;
    can_handle.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&can_handle) != HAL_OK)
    {
        return 1;
    }

    if (HAL_CAN_Start(&can_handle) != HAL_OK)
    {
        return 1;
    }

    return 0;
}

/* Configure CAN message filter */
int
can_phy_hal_set_filter (void)
{
    can_filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    can_filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    can_filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    can_filter.FilterActivation     = ENABLE;
    // List of IDs you want to receive
    uint16_t ids[] = {BOARD_CONF_CAN_SENSOR1_RX_ID, BOARD_CONF_CAN_SENSOR2_RX_ID, BOARD_CONF_CAN_SENSOR3_RX_ID, BOARD_CONF_CAN_STATUS_RX_ID};
    uint8_t num_ids = sizeof(ids) / sizeof(ids[0]);

    for (uint8_t i = 0; i < num_ids; ++i)
    {
        uint32_t std_id = ids[i] << 5;  // Shift to align with CAN register format

        can_filter.FilterBank = i;                          // One filter per ID
        can_filter.FilterMode = CAN_FILTERMODE_IDLIST;
        can_filter.FilterFIFOAssignment = CAN_RX_FIFO0;
        can_filter.FilterActivation = ENABLE;

        // Put only this ID into the filter (others zeroed)
        can_filter.FilterIdHigh     = (std_id << 5);
        can_filter.FilterIdLow      = 0;
        can_filter.FilterMaskIdHigh = std_id;
        can_filter.FilterMaskIdLow  = 0x0000;

        if (HAL_CAN_ConfigFilter(&can_handle, &can_filter) != HAL_OK)
        {
            return 1;  // Error
        }
    }
    HAL_CAN_ActivateNotification(&can_handle, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);

    return 0;
}

/* CAN RX FIFO0 message pending callback */
void
HAL_CAN_RxFifo0MsgPendingCallback (CAN_HandleTypeDef *hcan)
{
    uint8_t rx_data[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK)
    {
        if (rx_header.StdId == BOARD_CONF_CAN_SENSOR1_RX_ID)
        {
            isotp_on_can_message(&g_link, rx_data, rx_header.DLC);
        } else if(rx_header.StdId == BOARD_CONF_CAN_SENSOR2_RX_ID) {
            isotp_on_can_message(&g_link2, rx_data, rx_header.DLC);
        } else if(rx_header.StdId == BOARD_CONF_CAN_STATUS_RX_ID) {
            isotp_on_can_message(&comm_link,rx_data, rx_header.DLC);
        } else if(rx_header.StdId == BOARD_CONF_CAN_GLOBAL_BRDCAST_RX_ID) {
            session_mgmt_on_global_can_msg(rx_data, rx_header.DLC);
        } else if(rx_header.StdId == BOARD_CONF_CAN_SENSOR3_RX_ID) {
            isotp_on_can_message(&g_link3,rx_data, rx_header.DLC);
        }
    }
}

/* Transmit CAN frame */
size_t
can_phy_transmit (uint32_t arb_id, uint8_t *data, size_t len)
{
    CAN_TxHeaderTypeDef tx_header;

    tx_header.StdId = arb_id;
    tx_header.IDE   = CAN_ID_STD;
    tx_header.RTR   = CAN_RTR_DATA;
    tx_header.DLC   = (uint8_t)len;

    if (HAL_CAN_AddTxMessage(&can_handle, &tx_header, data, &tx_mailbox)
        != HAL_OK)
    {
        return 1;
    }

    while (HAL_CAN_IsTxMessagePending(&can_handle, tx_mailbox) != 0U)
    {
        /* Wait until the message is sent */
    }

    return len;
}

/* ISO-TP transport layer: Send CAN message */
int
isotp_user_send_can (const uint32_t arbitration_id,
                     const uint8_t *data,
                     const uint8_t  size)
{
    if ((data == NULL) || (size > 8U))
    {
        return -1;
    }

    size_t sent = can_phy_transmit(arbitration_id, (uint8_t *)data, size);

    return (sent == size) ? 0 : -1;
}

/* Return current time in microseconds */
uint32_t
isotp_user_get_us (void)
{
    return HAL_GetTick() * 1000U;
}

/* Print debug message to RTT */
void
isotp_user_debug (const char *message, ...)
{
    char    buffer[128];
    va_list args;

    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    SEGGER_RTT_WriteString(0, buffer);
    SEGGER_RTT_WriteString(0, "\n");
}

/* Initialize CAN pins */
manikin_status_t
init_can_pins (void)
{
    GPIO_InitTypeDef can_gpio;

    can_gpio.Mode  = GPIO_MODE_AF_PP;
    can_gpio.Pull  = GPIO_NOPULL;
    can_gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    /* Configure RX pin */
    can_gpio.Pin       = BOARD_CONF_CAN_RX_PIN;
    can_gpio.Alternate = BOARD_CONF_CAN_RX_PIN_MUX;
    HAL_GPIO_Init(BOARD_CONF_CAN_RX_PORT, &can_gpio);

    /* Configure TX pin */
    can_gpio.Pin       = BOARD_CONF_CAN_TX_PIN;
    can_gpio.Alternate = BOARD_CONF_CAN_TX_PIN_MUX;
    HAL_GPIO_Init(BOARD_CONF_CAN_TX_PORT, &can_gpio);

    return MANIKIN_STATUS_OK;
}

/* Full CAN initialization */
void
init_can (void)
{
    BOARD_CONF_CAN_GPIO_CLK_EN();
    BOARD_CONF_CAN_CLK_EN();

    (void)init_can_pins();

    (void)can_phy_hal_init_can_mcu(BOARD_CONF_CAN_SPEED);
    (void)can_phy_hal_set_filter();
    isotp_init_link(&g_link, BOARD_CONF_CAN_SENSOR1_TX_ID,
        g_isotpSendBuf, sizeof(g_isotpSendBuf), 
        g_isotpRecvBuf, sizeof(g_isotpRecvBuf));
    
    isotp_init_link(&g_link2, BOARD_CONF_CAN_SENSOR2_TX_ID,
            g_isotpSendBuf2, sizeof(g_isotpSendBuf2), 
            g_isotpRecvBuf2, sizeof(g_isotpRecvBuf2));
    isotp_init_link(&g_link3, BOARD_CONF_CAN_SENSOR3_TX_ID,
            g_isotpSendBuf4, sizeof(g_isotpSendBuf4), 
            g_isotpRecvBuf4, sizeof(g_isotpRecvBuf4));

    isotp_init_link(&comm_link, BOARD_CONF_CAN_STATUS_TX_ID,
                g_isotpSendBuf3, sizeof(g_isotpSendBuf3), 
                g_isotpRecvBuf3, sizeof(g_isotpRecvBuf3));
}

/* Reset CAN peripheral */
void
can_phy_reset_can (void)
{
    (void)HAL_CAN_Init(&can_handle);
    (void)HAL_CAN_Start(&can_handle);
}

/* Stop CAN peripheral */
void
can_phy_close_can (void)
{
    (void)HAL_CAN_Stop(&can_handle);
}
