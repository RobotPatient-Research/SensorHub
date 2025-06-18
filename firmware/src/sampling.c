#include "error_handler/error_handler.h"
#include "stdint.h"
#include <cmsis_gcc.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_gpio.h>

#include "i2c/i2c.h"
#include "board_conf.h"
#include "common/manikin_types.h"
#include "sample_timer/sample_timer.h"
#include "common/manikin_bit_manipulation.h"
#include "cli.h"
#include "lwrb/lwrb.h"
#include "usbd_cdc_if.h"
#include "can_wrapper.h"
#include "isotp.h"
#include "vl6180x/vl6180x.h"

/* Buffers */
static lwrb_t  buff;
static uint8_t buff_data[1024];

static uint8_t cbor_buff[512];
static uint8_t std_out_buf[1024];

/* Sample counters */
/* Sensor contexts */
/* Sensor sample flags */
/* External ISO-TP link */
/* Sample timer contexts */
#if BOARD_CONF_USE_SENSOR1
static manikin_sensor_ctx_t sensor1_ctx;
volatile uint8_t            sensor_timer_1_trigger;
extern IsoTpLink            g_link;
static sample_timer_ctx_t   timer1_ctx;
size_t                      sensor1_sample_id = 0;
static lwrb_t               can_buff;
static uint8_t              can_buff_data[512];
static uint8_t              can_out_data[128];
#endif
#if BOARD_CONF_USE_SENSOR2
static manikin_sensor_ctx_t sensor2_ctx;
volatile uint8_t            sensor_timer_2_trigger;
extern IsoTpLink            g_link2;
static sample_timer_ctx_t   timer2_ctx;
size_t                      sensor2_sample_id = 0;
static lwrb_t               can_buff2;
static uint8_t              can_buff2_data[512];
static uint8_t              can_out2_data[128];
#endif
#if BOARD_CONF_USE_SENSOR3
static manikin_sensor_ctx_t sensor3_ctx;
volatile uint8_t            sensor_timer_3_trigger;
extern IsoTpLink            g_link3;
static sample_timer_ctx_t   timer3_ctx;
size_t                      sensor3_sample_id = 0;
static lwrb_t               can_buff3;
static uint8_t              can_buff3_data[512];
static uint8_t              can_out3_data[128];
#endif

/* CAN data packaging */
typedef struct __attribute__((__packed__))
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR1_SAMPLE_T data;
} sample_sensor1_t;

typedef struct __attribute__((__packed__))
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR2_SAMPLE_T data;
} sample_sensor2_t;

typedef struct __attribute__((__packed__))
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR3_SAMPLE_T data;
} sample_sensor3_t;

/* Timer interrupt handler */
void
sample_irq (TIM_TypeDef *tim)
{
#if BOARD_CONF_USE_SENSOR1
    if (tim == BOARD_CONF_TIMER_SENSOR_1)
    {
        sensor_timer_1_trigger = 1U;
    }
#endif

#if BOARD_CONF_USE_SENSOR2
    if (tim == BOARD_CONF_TIMER_SENSOR_2)
    {
        sensor_timer_2_trigger = 1U;
    }
#endif
#if BOARD_CONF_USE_SENSOR3
    if (tim == BOARD_CONF_TIMER_SENSOR_2)
    {
        sensor_timer_3_trigger = 1U;
    }
#endif
}

/* I2C0 pin init */
static manikin_status_t
init_i2c0_pins (void)
{
    GPIO_InitTypeDef pin_init;

    pin_init.Mode  = GPIO_MODE_AF_OD;
    pin_init.Pull  = GPIO_NOPULL;
    pin_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    pin_init.Pin       = BOARD_CONF_I2C0_SDA_PIN;
    pin_init.Alternate = BOARD_CONF_I2C0_SDA_PIN_MUX;
    HAL_GPIO_Init(BOARD_CONF_I2C0_SDA_PORT, &pin_init);

    pin_init.Pin       = BOARD_CONF_I2C0_SCL_PIN;
    pin_init.Alternate = BOARD_CONF_I2C0_SCL_PIN_MUX;
    HAL_GPIO_Init(BOARD_CONF_I2C0_SCL_PORT, &pin_init);

    pin_init.Pin       = BOARD_CONF_SENSOR1_RESET_PIN;
    pin_init.Mode      = GPIO_MODE_OUTPUT_PP;
    pin_init.Alternate = 0;
    pin_init.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOARD_CONF_SENSOR1_RESET_PORT, &pin_init);
    HAL_GPIO_WritePin(BOARD_CONF_SENSOR1_RESET_PORT,
                      BOARD_CONF_SENSOR1_RESET_PIN,
                      GPIO_PIN_SET);

    BOARD_CONF_I2C0_CLK_EN();

    return MANIKIN_STATUS_OK;
}

/* I2C1 pin init */
static manikin_status_t
init_i2c1_pins (void)
{
    GPIO_InitTypeDef pin_init;

    pin_init.Mode  = GPIO_MODE_AF_OD;
    pin_init.Pull  = GPIO_NOPULL;
    pin_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    pin_init.Pin       = BOARD_CONF_I2C1_SDA_PIN;
    pin_init.Alternate = BOARD_CONF_I2C1_SDA_PIN_MUX;
    HAL_GPIO_Init(BOARD_CONF_I2C1_SDA_PORT, &pin_init);

    pin_init.Pin       = BOARD_CONF_I2C1_SCL_PIN;
    pin_init.Alternate = BOARD_CONF_I2C1_SCL_PIN_MUX;
    HAL_GPIO_Init(BOARD_CONF_I2C1_SCL_PORT, &pin_init);

    pin_init.Pin       = BOARD_CONF_SENSOR2_RESET_PIN;
    pin_init.Mode      = GPIO_MODE_OUTPUT_PP;
    pin_init.Alternate = 0;
    pin_init.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOARD_CONF_SENSOR2_RESET_PORT, &pin_init);
    HAL_GPIO_WritePin(BOARD_CONF_SENSOR2_RESET_PORT,
                      BOARD_CONF_SENSOR2_RESET_PIN,
                      GPIO_PIN_SET);
    BOARD_CONF_I2C1_CLK_EN();
    return MANIKIN_STATUS_OK;
}

/* Sensor 1 init */
static manikin_status_t
init_i2c_sensor1 (void)
{
    sensor_timer_1_trigger = 0U;

    manikin_i2c_init(BOARD_CONF_I2C0_INSTANCE, BOARD_CONF_I2C0_SPEED);
    sensor1_ctx.i2c      = BOARD_CONF_I2C0_INSTANCE;
    sensor1_ctx.i2c_addr = BOARD_CONF_SENSOR1_ADDR;
    timer1_ctx.frequency = BOARD_CONF_SENSOR1_SAMPLE_RATE_HZ;
    timer1_ctx.timer     = BOARD_CONF_TIMER_SENSOR_1;
    timer1_ctx.watchdog  = WWDG;

    manikin_status_t status = BOARD_CONF_SENSOR1_INIT(&sensor1_ctx);
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&timer1_ctx);
    }

    return MANIKIN_STATUS_OK;
}

/* Sensor 2 init */
static manikin_status_t
init_i2c_sensor2 (void)
{
    manikin_i2c_init(BOARD_CONF_I2C1_INSTANCE, BOARD_CONF_I2C1_SPEED);
    sensor2_ctx.i2c      = BOARD_CONF_I2C1_INSTANCE;
    sensor2_ctx.i2c_addr = BOARD_CONF_SENSOR2_ADDR;
    timer2_ctx.frequency = BOARD_CONF_SENSOR2_SAMPLE_RATE_HZ;
    timer2_ctx.timer     = BOARD_CONF_TIMER_SENSOR_2;
    timer2_ctx.watchdog  = WWDG;

    manikin_status_t status = BOARD_CONF_SENSOR2_INIT(&sensor2_ctx);
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&timer2_ctx);
    }

    return MANIKIN_STATUS_OK;
}

#if BOARD_CONF_USE_SENSOR3
static manikin_status_t
init_i2c_sensor3 (void)
{
    sensor3_ctx.i2c      = BOARD_CONF_I2C1_INSTANCE;
    sensor3_ctx.i2c_addr = BOARD_CONF_SENSOR3_ADDR;
    timer3_ctx.frequency = BOARD_CONF_SENSOR3_SAMPLE_RATE_HZ;
    timer3_ctx.timer     = BOARD_CONF_TIMER_SENSOR_3;
    timer3_ctx.watchdog  = WWDG;

    manikin_status_t status = BOARD_CONF_SENSOR3_INIT(&sensor3_ctx);
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&timer3_ctx);
    }

    return MANIKIN_STATUS_OK;
}
#endif

manikin_status_t
start_sensor_sampling (void)
{
    manikin_status_t status;

    status = sample_timer_start(&timer1_ctx);
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);

#if BOARD_CONF_USE_SENSOR2
    status = sample_timer_start(&timer2_ctx);
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

#if BOARD_CONF_USE_SENSOR3
    status = sample_timer_start(&timer3_ctx);
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

    return MANIKIN_STATUS_OK;
}

manikin_status_t
stop_sensor_sampling (void)
{
    manikin_status_t status;

    status = sample_timer_stop(&timer1_ctx);
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);

#if BOARD_CONF_USE_SENSOR2
    status = sample_timer_stop(&timer2_ctx);
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

#if BOARD_CONF_USE_SENSOR3
    status = sample_timer_stop(&timer3_ctx);
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

    return MANIKIN_STATUS_OK;
}

/* Peripheral and sensor initialization */
manikin_status_t
init_peripherals_for_sensors (void)
{
    lwrb_init(&buff, buff_data, sizeof(buff_data));
    lwrb_init(&can_buff, can_buff_data, sizeof(can_buff_data));
    lwrb_init(&can_buff2, can_buff2_data, sizeof(can_buff2_data));
    lwrb_init(&can_buff3, can_buff3_data, sizeof(can_buff3_data));
#if BOARD_CONF_USE_SENSOR1
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    init_i2c0_pins();
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_1_EN();
    init_i2c_sensor1();
#endif

#if BOARD_CONF_USE_SENSOR2
    init_i2c1_pins();
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_2_EN();
    init_i2c_sensor2();
#endif

#if BOARD_CONF_USE_SENSOR3
    init_i2c_sensor3();
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_3_EN();
    init_i2c_sensor3();
#endif

    return MANIKIN_STATUS_OK;
}

/* Sensor 1 sampling logic */
manikin_status_t
check_and_sample_sensor1 (uint8_t *data_buf)
{
    sample_sensor1_t sample;
    memcpy(sample.sensor_name,
           BOARD_CONF_SENSOR1_NAME,
           sizeof(BOARD_CONF_SENSOR1_NAME));
    if (sensor_timer_1_trigger)
    {
        manikin_status_t status
            = sample_timer_start_cb_handler(&timer1_ctx, &sensor1_ctx);
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR1_SAMPLE(&sensor1_ctx, data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor1_sample_id++;

                __disable_irq();
                size_t len
                    = manikin_cli_on_new_sensor_sample(cbor_buff,
                                                       sizeof(cbor_buff),
                                                       BOARD_CONF_SENSOR1_NAME,
                                                       sensor1_sample_id,
                                                       data_buf,
                                                       1);

                lwrb_write(&buff, cbor_buff, len);

                BOARD_CONF_SENSOR1_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor1_sample_id;
                lwrb_write(&can_buff, &sample, sizeof(sample_sensor1_t));
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(&timer1_ctx, &sensor1_ctx, status);
        sensor_timer_1_trigger = 0U;
    }

    return MANIKIN_STATUS_OK;
}

/* Sensor 2 sampling logic */
manikin_status_t
check_and_sample_sensor2 (uint8_t *data_buf)
{
    sample_sensor2_t sample;
    memcpy(sample.sensor_name,
           BOARD_CONF_SENSOR2_NAME,
           sizeof(BOARD_CONF_SENSOR2_NAME));
    if (sensor_timer_2_trigger)
    {
        manikin_status_t status
            = sample_timer_start_cb_handler(&timer2_ctx, &sensor2_ctx);
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR2_SAMPLE(&sensor2_ctx, data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor2_sample_id++;

                __disable_irq();
                size_t len
                    = manikin_cli_on_new_sensor_sample(cbor_buff,
                                                       sizeof(cbor_buff),
                                                       BOARD_CONF_SENSOR2_NAME,
                                                       sensor2_sample_id,
                                                       data_buf,
                                                       16);

                BOARD_CONF_SENSOR2_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor2_sample_id;
                lwrb_write(&can_buff2, &sample, sizeof(sample_sensor2_t));
                lwrb_write(&buff, cbor_buff, len);
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(&timer2_ctx, &sensor2_ctx, status);
        sensor_timer_2_trigger = 0U;
    }

    return MANIKIN_STATUS_OK;
}

/* Placeholder for optional sensor 3 */
manikin_status_t
check_and_sample_sensor3 (uint8_t *data_buf)
{
    sample_sensor3_t sample3;
    memcpy(sample3.sensor_name,
           BOARD_CONF_SENSOR3_NAME,
           sizeof(BOARD_CONF_SENSOR3_NAME));
    if (sensor_timer_3_trigger)
    {
        manikin_status_t status
            = sample_timer_start_cb_handler(&timer3_ctx, &sensor3_ctx);
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR3_SAMPLE(&sensor3_ctx, data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor3_sample_id++;

                __disable_irq();
                size_t len
                    = manikin_cli_on_new_sensor_sample(cbor_buff,
                                                       sizeof(cbor_buff),
                                                       BOARD_CONF_SENSOR3_NAME,
                                                       sensor3_sample_id,
                                                       data_buf,
                                                       16);

                BOARD_CONF_SENSOR3_SAMPLE_PARSE(data_buf, &(sample3.data));
                sample3.frame_id = sensor3_sample_id;
                lwrb_write(&can_buff3, &sample3, sizeof(sample_sensor3_t));
                lwrb_write(&buff, cbor_buff, len);
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(&timer3_ctx, &sensor3_ctx, status);
        sensor_timer_3_trigger = 0U;
    }

    return MANIKIN_STATUS_OK;
}

/* Send data over USB CDC */
manikin_status_t
print_to_stdout (void)
{
    uint32_t len = lwrb_read(&buff, std_out_buf, sizeof(std_out_buf));
    (void)CDC_Transmit_FS(std_out_buf, len);
    return MANIKIN_STATUS_OK;
}

/* Send data over CAN using ISO-TP */
manikin_status_t
print_to_can (const char *sensor_id, uint8_t *data_buf, size_t buffer_size)
{
    (void)sensor_id;
    (void)data_buf;
    (void)buffer_size;

    uint32_t len = lwrb_read(&can_buff, can_out_data, sizeof(sample_sensor1_t));
    if (len != 0U)
    {
        (void)isotp_send(&g_link, can_out_data, len);
    }
    len = lwrb_read(&can_buff2, can_out2_data, sizeof(sample_sensor2_t));
    if (len != 0U)
    {
        (void)isotp_send(&g_link2, can_out2_data, len);
    }
    len = lwrb_read(&can_buff3, can_out3_data, sizeof(sample_sensor3_t));
    if (len != 0U)
    {
        (void)isotp_send(&g_link3, can_out3_data, len);
    }

    return MANIKIN_STATUS_OK;
}
