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

#define MAX_SAMPLE_SIZE                                         \
    ((sizeof(sample_sensor1_t) > sizeof(sample_sensor2_t))      \
         ? (sizeof(sample_sensor1_t) > sizeof(sample_sensor3_t) \
                ? sizeof(sample_sensor1_t)                      \
                : sizeof(sample_sensor3_t))                     \
         : (sizeof(sample_sensor2_t) > sizeof(sample_sensor3_t) \
                ? sizeof(sample_sensor2_t)                      \
                : sizeof(sample_sensor3_t)))

/* Buffers */
static lwrb_t  buff;
static uint8_t buff_data[1024];

static uint8_t cbor_buff[512];
static uint8_t std_out_buf[1024];

struct sensor_state
{
    sample_timer_ctx_t   timer_ctx;
    manikin_sensor_ctx_t sensor_ctx;
    IsoTpLink            iso_tp_link;
    size_t               sample_id;
    lwrb_t               can_ringbuffer;
    uint8_t              can_ringbuffer_data[512];
};


#if BOARD_CONF_USE_SENSOR1
struct sensor_state sensor1 = { 0 };
volatile uint8_t    sensor_timer_1_trigger;
#endif
#if BOARD_CONF_USE_SENSOR2
struct sensor_state sensor2 = { 0 };
volatile uint8_t    sensor_timer_2_trigger;
#endif
#if BOARD_CONF_USE_SENSOR3
struct sensor_state sensor3 = { 0 };
volatile uint8_t    sensor_timer_3_trigger;

#endif

/* CAN data packaging */
typedef struct
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR1_SAMPLE_T data;
} sample_sensor1_t;

typedef struct
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR2_SAMPLE_T data;
} sample_sensor2_t;

typedef struct
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
    if (tim == BOARD_CONF_TIMER_SENSOR_3)
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

static manikin_status_t
init_i2c_sensor_struct (struct sensor_state *sensor,
                        I2C_TypeDef         *i2c_inst,
                        uint8_t              i2c_addr,
                        uint16_t             sample_freq,
                        TIM_TypeDef         *timer,
                        WWDG_TypeDef        *watchdog)
{
    sensor->sensor_ctx.i2c      = i2c_inst;
    sensor->sensor_ctx.i2c_addr = i2c_addr;
    sensor->timer_ctx.frequency = sample_freq;
    sensor->timer_ctx.timer     = timer;
    sensor->timer_ctx.watchdog  = watchdog;

    return MANIKIN_STATUS_OK;
}

manikin_status_t
start_sensor_sampling (void)
{
    manikin_status_t status;

    status = sample_timer_start(&(sensor1.timer_ctx));
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);

#if BOARD_CONF_USE_SENSOR2
    status = sample_timer_start(&(sensor2.timer_ctx));
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

#if BOARD_CONF_USE_SENSOR3
    status = sample_timer_start(&(sensor3.timer_ctx));
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

    return MANIKIN_STATUS_OK;
}

manikin_status_t
stop_sensor_sampling (void)
{
    manikin_status_t status;

    status = sample_timer_stop(&(sensor1.timer_ctx));
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);

#if BOARD_CONF_USE_SENSOR2
    status = sample_timer_stop(&(sensor2.timer_ctx));
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

#if BOARD_CONF_USE_SENSOR3
    status = sample_timer_stop(&(sensor3.timer_ctx));
    MANIKIN_ASSERT(
        0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

    return MANIKIN_STATUS_OK;
}

/* Peripheral and sensor initialization */
manikin_status_t
init_peripherals_for_sensors (void)
{
    manikin_status_t status;
    lwrb_init(&buff, buff_data, sizeof(buff_data));
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    init_i2c0_pins();
    init_i2c1_pins();

#if BOARD_CONF_USE_SENSOR1
    lwrb_init(&(sensor1.can_ringbuffer),
              (sensor1.can_ringbuffer_data),
              sizeof(sensor1.can_ringbuffer_data));
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_1_EN();
    init_i2c_sensor_struct(&sensor1,
                           BOARD_CONF_I2C1_INSTANCE,
                           BOARD_CONF_SENSOR1_ADDR,
                           BOARD_CONF_SENSOR1_SAMPLE_RATE_HZ,
                           BOARD_CONF_TIMER_SENSOR_1,
                           WWDG);
    status = BOARD_CONF_SENSOR1_INIT(&(sensor1.sensor_ctx));
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&(sensor1.timer_ctx));
    }
#endif

#if BOARD_CONF_USE_SENSOR2
    lwrb_init(&(sensor2.can_ringbuffer),
              (sensor2.can_ringbuffer_data),
              sizeof(sensor2.can_ringbuffer_data));
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_2_EN();
    init_i2c_sensor_struct(&sensor2,
                           BOARD_CONF_I2C0_INSTANCE,
                           BOARD_CONF_SENSOR2_ADDR,
                           BOARD_CONF_SENSOR2_SAMPLE_RATE_HZ,
                           BOARD_CONF_TIMER_SENSOR_2,
                           WWDG);
    status = BOARD_CONF_SENSOR2_INIT(&(sensor2.sensor_ctx));
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&(sensor2.timer_ctx));
    }
#endif

#if BOARD_CONF_USE_SENSOR3
    lwrb_init(&(sensor3.can_ringbuffer),
              (sensor3.can_ringbuffer_data),
              sizeof(sensor3.can_ringbuffer_data));
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_3_EN();
    init_i2c_sensor_struct(&sensor3,
                           BOARD_CONF_I2C0_INSTANCE,
                           BOARD_CONF_SENSOR3_ADDR,
                           BOARD_CONF_SENSOR3_SAMPLE_RATE_HZ,
                           BOARD_CONF_TIMER_SENSOR_3,
                           WWDG);
    status = BOARD_CONF_SENSOR3_INIT(&(sensor3.sensor_ctx));
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&(sensor3.timer_ctx));
    }
#endif

    return MANIKIN_STATUS_OK;
}

manikin_status_t
check_and_sample_sensor1 (uint8_t *data_buf)
{
    sample_sensor1_t sample;
    memcpy(sample.sensor_name,
           BOARD_CONF_SENSOR1_NAME,
           sizeof(BOARD_CONF_SENSOR1_NAME));
    manikin_status_t status = MANIKIN_STATUS_OK;
    if (sensor_timer_1_trigger)
    {
        status = sample_timer_start_cb_handler(
            &(sensor1.timer_ctx), &(sensor1.sensor_ctx));
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR1_SAMPLE(&(sensor1.sensor_ctx), data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor1.sample_id++;

                size_t len
                    = manikin_cli_on_new_sensor_sample(cbor_buff,
                                                       sizeof(cbor_buff),
                                                       BOARD_CONF_SENSOR1_NAME,
                                                       sensor1.sample_id,
                                                       data_buf,
                                                       1);
                __disable_irq();
                lwrb_write(&buff, cbor_buff, len);
                __enable_irq();

                BOARD_CONF_SENSOR1_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor1.sample_id;

                __disable_irq();
                lwrb_write(&(sensor1.can_ringbuffer),
                           &sample,
                           sizeof(sample_sensor1_t));
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(
            &(sensor1.timer_ctx), &(sensor1.sensor_ctx), status);
        sensor_timer_1_trigger = 0U;
    }

    return MANIKIN_STATUS_OK;
}

manikin_status_t
check_and_sample_sensor2 (uint8_t *data_buf)
{
    sample_sensor2_t sample;
    memcpy(sample.sensor_name,
           BOARD_CONF_SENSOR2_NAME,
           sizeof(BOARD_CONF_SENSOR2_NAME));
    manikin_status_t status = MANIKIN_STATUS_OK;
    if (sensor_timer_2_trigger)
    {
        status = sample_timer_start_cb_handler(
            &(sensor2.timer_ctx), &(sensor2.sensor_ctx));
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR2_SAMPLE(&(sensor2.sensor_ctx), data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor2.sample_id++;

                size_t len
                    = manikin_cli_on_new_sensor_sample(cbor_buff,
                                                       sizeof(cbor_buff),
                                                       BOARD_CONF_SENSOR2_NAME,
                                                       sensor2.sample_id,
                                                       data_buf,
                                                       1);
                __disable_irq();
                lwrb_write(&buff, cbor_buff, len);
                __enable_irq();

                BOARD_CONF_SENSOR2_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor2.sample_id;

                __disable_irq();
                lwrb_write(&(sensor2.can_ringbuffer),
                           &sample,
                           sizeof(sample_sensor2_t));
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(
            &(sensor2.timer_ctx), &(sensor2.sensor_ctx), status);
        sensor_timer_2_trigger = 0U;
    }

    return status;
}

manikin_status_t
check_and_sample_sensor3 (uint8_t *data_buf)
{
    sample_sensor3_t sample;
    memcpy(sample.sensor_name,
           BOARD_CONF_SENSOR3_NAME,
           sizeof(BOARD_CONF_SENSOR3_NAME));
    manikin_status_t status = MANIKIN_STATUS_OK;
    if (sensor_timer_3_trigger)
    {

        status = sample_timer_start_cb_handler(&(sensor3.timer_ctx),
                                               &(sensor3.sensor_ctx));
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR3_SAMPLE(&(sensor3.sensor_ctx), data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor3.sample_id++;

                size_t len
                    = manikin_cli_on_new_sensor_sample(cbor_buff,
                                                       sizeof(cbor_buff),
                                                       BOARD_CONF_SENSOR3_NAME,
                                                       sensor3.sample_id,
                                                       data_buf,
                                                       1);
                __disable_irq();
                lwrb_write(&buff, cbor_buff, len);
                __enable_irq();

                BOARD_CONF_SENSOR3_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor3.sample_id;

                __disable_irq();
                lwrb_write(&(sensor3.can_ringbuffer),
                           &sample,
                           sizeof(sample_sensor3_t));
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(
            &(sensor3.timer_ctx), &(sensor3.sensor_ctx), status);
        sensor_timer_3_trigger = 0U;
    }

    return status;
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
print_to_can (void)
{
    uint8_t  read_buf[MAX_SAMPLE_SIZE];
    uint32_t len = lwrb_read(
        &(sensor1.can_ringbuffer), read_buf, sizeof(sample_sensor1_t));
    if (len != 0U)
    {
        (void)isotp_send(&(sensor1.iso_tp_link), read_buf, len);
    }
    len = lwrb_read(
        &(sensor2.can_ringbuffer), read_buf, sizeof(sample_sensor2_t));
    if (len != 0U)
    {
        (void)isotp_send(&(sensor2.iso_tp_link), read_buf, len);
    }
    len = lwrb_read(
        &(sensor3.can_ringbuffer), read_buf, sizeof(sample_sensor3_t));
    if (len != 0U)
    {
        (void)isotp_send(&(sensor3.iso_tp_link), read_buf, len);
    }

    return MANIKIN_STATUS_OK;
}
