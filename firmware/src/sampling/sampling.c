#include "error_handler/error_handler.h"
#include "stdint.h"
#include <cmsis_gcc.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_gpio.h>
#include "sampling.h"
#include "board_conf.h"
#include "common/manikin_types.h"
#include "sample_timer/sample_timer.h"
#include "cli.h"
#include "lwrb/lwrb.h"
#include "isotp.h"
#include "private/sampling_data_types.h"
#include "manikin_platform.h"

#define FILE_HASH 0xaf54d256

#if BOARD_CONF_USE_SENSOR3
#define MAX_SAMPLE_SIZE                                                                                                \
    ((sizeof(sample_sensor1_t) > sizeof(sample_sensor2_t))                                                             \
         ? (sizeof(sample_sensor1_t) > sizeof(sample_sensor3_t) ? sizeof(sample_sensor1_t) : sizeof(sample_sensor3_t)) \
         : (sizeof(sample_sensor2_t) > sizeof(sample_sensor3_t) ? sizeof(sample_sensor2_t)                             \
                                                          : sizeof(sample_sensor3_t)))
#elif BOARD_CONF_USE_SENSOR2
#define MAX_SAMPLE_SIZE \
    ((sizeof(sample_sensor1_t) > sizeof(sample_sensor2_t)) ? sizeof(sample_sensor1_t) : sizeof(sample_sensor2_t))

#elif BOARD_CONF_USE_SENSOR1
#define MAX_SAMPLE_SIZE (sizeof(sample_sensor1_t))
#endif

#if BOARD_CONF_USE_SENSOR1
struct sensor_state sensor1 = { 0U };
volatile uint8_t    sensor_timer_1_trigger;
#endif
#if BOARD_CONF_USE_SENSOR2
struct sensor_state sensor2 = { 0U };
volatile uint8_t    sensor_timer_2_trigger;
#endif
#if BOARD_CONF_USE_SENSOR3
struct sensor_state sensor3 = { 0U };
volatile uint8_t    sensor_timer_3_trigger;

#endif

/* Timer interrupt handler */
void
sample_irq (TIM_TypeDef *tim)
{
#if BOARD_CONF_USE_SENSOR1
    if (BOARD_CONF_TIMER_SENSOR_1 == tim)
    {
        sensor_timer_1_trigger = 1U;
    }
#endif

#if BOARD_CONF_USE_SENSOR2
    if (BOARD_CONF_TIMER_SENSOR_2 == tim)
    {
        sensor_timer_2_trigger = 1U;
    }
#endif
#if BOARD_CONF_USE_SENSOR3
    if (BOARD_CONF_TIMER_SENSOR_3 == tim)
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
    HAL_GPIO_WritePin(BOARD_CONF_SENSOR1_RESET_PORT, BOARD_CONF_SENSOR1_RESET_PIN, GPIO_PIN_SET);

    BOARD_CONF_I2C0_CLK_EN();

    return MANIKIN_STATUS_OK;
}
#if BOARD_CONF_USE_SENSOR2
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
    HAL_GPIO_WritePin(BOARD_CONF_SENSOR2_RESET_PORT, BOARD_CONF_SENSOR2_RESET_PIN, GPIO_PIN_SET);
    BOARD_CONF_I2C1_CLK_EN();
    return MANIKIN_STATUS_OK;
}
#endif
static inline manikin_status_t
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
    MANIKIN_I2C_HAL_INIT(i2c_inst, BOARD_CONF_I2C1_SPEED);
    return MANIKIN_STATUS_OK;
}

manikin_status_t
start_sensor_sampling (void)
{
    manikin_status_t status;

    status = sample_timer_start(&(sensor1.timer_ctx));
    MANIKIN_ASSERT(0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);

#if BOARD_CONF_USE_SENSOR2
    status = sample_timer_start(&(sensor2.timer_ctx));
    MANIKIN_ASSERT(0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

#if BOARD_CONF_USE_SENSOR3
    status = sample_timer_start(&(sensor3.timer_ctx));
    MANIKIN_ASSERT(0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

    return MANIKIN_STATUS_OK;
}

manikin_status_t
stop_sensor_sampling (void)
{
    manikin_status_t status;

    status = sample_timer_stop(&(sensor1.timer_ctx));
    MANIKIN_ASSERT(0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);

#if BOARD_CONF_USE_SENSOR2
    status = sample_timer_stop(&(sensor2.timer_ctx));
    MANIKIN_ASSERT(0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

#if BOARD_CONF_USE_SENSOR3
    status = sample_timer_stop(&(sensor3.timer_ctx));
    MANIKIN_ASSERT(0x01, status == MANIKIN_STATUS_OK, MANIKIN_STATUS_ERR_SENSOR_INIT_FAIL);
#endif

    return MANIKIN_STATUS_OK;
}

/* Peripheral and sensor initialization */
manikin_status_t
init_peripherals_for_sensors (void)
{
    manikin_status_t status;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    init_i2c0_pins();
#if BOARD_CONF_USE_SENSOR2
    init_i2c1_pins();
#endif
#if BOARD_CONF_USE_SENSOR1
    lwrb_init(&(sensor1.can_ringbuffer), (sensor1.can_ringbuffer_data), sizeof(sensor1.can_ringbuffer_data));
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_1_EN();
    init_i2c_sensor_struct(&sensor1,
                           BOARD_CONF_I2C0_INSTANCE,
                           BOARD_CONF_SENSOR1_ADDR,
                           BOARD_CONF_SENSOR1_SAMPLE_RATE_HZ,
                           BOARD_CONF_TIMER_SENSOR_1,
                           WWDG);
    status = BOARD_CONF_SENSOR1_INIT(&(sensor1.sensor_ctx));
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&(sensor1.timer_ctx));
        printf("Sensor 1 init success!\r\n");
    }
    else
    {
        printf("Sensor 1 init unsuccessful!\r\n");
        non_critical_error(FILE_HASH, __LINE__);
    }
#endif

#if BOARD_CONF_USE_SENSOR2
    lwrb_init(&(sensor2.can_ringbuffer), (sensor2.can_ringbuffer_data), sizeof(sensor2.can_ringbuffer_data));
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_2_EN();
    init_i2c_sensor_struct(&sensor2,
                           BOARD_CONF_I2C1_INSTANCE,
                           BOARD_CONF_SENSOR2_ADDR,
                           BOARD_CONF_SENSOR2_SAMPLE_RATE_HZ,
                           BOARD_CONF_TIMER_SENSOR_2,
                           WWDG);
    status = BOARD_CONF_SENSOR2_INIT(&(sensor2.sensor_ctx));
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&(sensor2.timer_ctx));
        printf("Sensor 2 init success!\r\n");
    }
    else
    {
        printf("Sensor 2 init unsuccessful!\r\n");
        non_critical_error(FILE_HASH, __LINE__);
    }
#endif

#if BOARD_CONF_USE_SENSOR3
    lwrb_init(&(sensor3.can_ringbuffer), (sensor3.can_ringbuffer_data), sizeof(sensor3.can_ringbuffer_data));
    HAL_Delay(1000U);
    BOARD_CONF_TIMER_SENSOR_3_EN();
    init_i2c_sensor_struct(&sensor3,
                           BOARD_CONF_I2C1_INSTANCE,
                           BOARD_CONF_SENSOR3_ADDR,
                           BOARD_CONF_SENSOR3_SAMPLE_RATE_HZ,
                           BOARD_CONF_TIMER_SENSOR_3,
                           WWDG);
    status = BOARD_CONF_SENSOR3_INIT(&(sensor3.sensor_ctx));
    if (status == MANIKIN_STATUS_OK)
    {
        sample_timer_init(&(sensor3.timer_ctx));
        printf("Sensor 3 init success!\r\n");
    }
    else
    {
        printf("Sensor 3 init unsuccessful!\r\n");
        non_critical_error(FILE_HASH, __LINE__);
    }
#endif

    return MANIKIN_STATUS_OK;
}

#if BOARD_CONF_USE_SENSOR1
manikin_status_t
check_and_sample_sensor1 ()
{
    sample_sensor1_t sample;
    uint8_t          data_buf[2 * sizeof(sample_sensor1_t)];
    memcpy(sample.sensor_name, BOARD_CONF_SENSOR1_NAME, sizeof(BOARD_CONF_SENSOR1_NAME));
    manikin_status_t status = MANIKIN_STATUS_OK;
    if (sensor_timer_1_trigger)
    {
        status = sample_timer_start_cb_handler(&(sensor1.timer_ctx), &(sensor1.sensor_ctx));
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR1_SAMPLE(&(sensor1.sensor_ctx), data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor1.sample_id++;

                size_t len = manikin_cli_on_new_sensor_sample(
                    BOARD_CONF_SENSOR1_NAME, sensor1.sample_id, data_buf, sizeof(sample_sensor1_t));
                if (len < sizeof(sample_sensor1_t))
                {
                    return MANIKIN_STATUS_ERR_CONVERSION_FAILED;
                }
                BOARD_CONF_SENSOR1_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor1.sample_id;

                __disable_irq();
                lwrb_write(&(sensor1.can_ringbuffer), &sample, sizeof(sample_sensor1_t));
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(&(sensor1.timer_ctx), &(sensor1.sensor_ctx), status);
        sensor_timer_1_trigger = 0U;
    }

    return MANIKIN_STATUS_OK;
}
#endif

#if BOARD_CONF_USE_SENSOR2
manikin_status_t
check_and_sample_sensor2 ()
{
    sample_sensor2_t sample;
    uint8_t          data_buf[2 * sizeof(sample_sensor2_t)];
    memcpy(sample.sensor_name, BOARD_CONF_SENSOR2_NAME, sizeof(BOARD_CONF_SENSOR2_NAME));
    manikin_status_t status = MANIKIN_STATUS_OK;
    if (sensor_timer_2_trigger)
    {
        status = sample_timer_start_cb_handler(&(sensor2.timer_ctx), &(sensor2.sensor_ctx));
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR2_SAMPLE(&(sensor2.sensor_ctx), data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor2.sample_id++;

                size_t len = manikin_cli_on_new_sensor_sample(
                    BOARD_CONF_SENSOR2_NAME, sensor2.sample_id, data_buf, sizeof(sample_sensor2_t));
                if (len < sizeof(sample_sensor2_t))
                {
                    return MANIKIN_STATUS_ERR_CONVERSION_FAILED;
                }
                BOARD_CONF_SENSOR2_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor2.sample_id;

                __disable_irq();
                lwrb_write(&(sensor2.can_ringbuffer), &sample, sizeof(sample_sensor2_t));
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(&(sensor2.timer_ctx), &(sensor2.sensor_ctx), status);
        sensor_timer_2_trigger = 0U;
    }

    return status;
}
#endif

#if BOARD_CONF_USE_SENSOR3
manikin_status_t
check_and_sample_sensor3 ()
{
    sample_sensor3_t sample;
    uint8_t          data_buf[2 * sizeof(sample_sensor3_t)];
    memcpy(sample.sensor_name, BOARD_CONF_SENSOR3_NAME, sizeof(BOARD_CONF_SENSOR3_NAME));
    manikin_status_t status = MANIKIN_STATUS_OK;
    if (sensor_timer_3_trigger)
    {

        status = sample_timer_start_cb_handler(&(sensor3.timer_ctx), &(sensor3.sensor_ctx));
        if (status == MANIKIN_STATUS_OK)
        {
            status = BOARD_CONF_SENSOR3_SAMPLE(&(sensor3.sensor_ctx), data_buf);
            if (status == MANIKIN_STATUS_OK)
            {
                sensor3.sample_id++;

                size_t len = manikin_cli_on_new_sensor_sample(
                    BOARD_CONF_SENSOR3_NAME, sensor3.sample_id, data_buf, sizeof(sample_sensor3_t));
                if (len < sizeof(sample_sensor3_t))
                {
                    return MANIKIN_STATUS_ERR_CONVERSION_FAILED;
                }
                BOARD_CONF_SENSOR3_SAMPLE_PARSE(data_buf, &(sample.data));
                sample.frame_id = sensor3.sample_id;

                __disable_irq();
                lwrb_write(&(sensor3.can_ringbuffer), &sample, sizeof(sample_sensor3_t));
                __enable_irq();
            }
        }

        sample_timer_end_cb_handler(&(sensor3.timer_ctx), &(sensor3.sensor_ctx), status);
        sensor_timer_3_trigger = 0U;
    }

    return status;
}
#endif

manikin_status_t
print_to_can (void)
{
    uint8_t  read_buf[MAX_SAMPLE_SIZE];
    #if BOARD_CONF_USE_SENSOR1
    uint32_t len = lwrb_read(&(sensor1.can_ringbuffer), read_buf, sizeof(sample_sensor1_t));
    if (len != 0U)
    {
        (void)isotp_send(&(sensor1.iso_tp_link), read_buf, len);
    }
    #endif
    #if BOARD_CONF_USE_SENSOR2
    len = lwrb_read(&(sensor2.can_ringbuffer), read_buf, sizeof(sample_sensor2_t));
    if (len != 0U)
    {
        (void)isotp_send(&(sensor2.iso_tp_link), read_buf, len);
    }
    #endif
    #if BOARD_CONF_USE_SENSOR3
    len = lwrb_read(&(sensor3.can_ringbuffer), read_buf, sizeof(sample_sensor3_t));
    if (len != 0U)
    {
        (void)isotp_send(&(sensor3.iso_tp_link), read_buf, len);
    }
    #endif
    return MANIKIN_STATUS_OK;
}

manikin_status_t
check_and_sample_sensors ()
{
#if BOARD_CONF_USE_SENSOR1
    check_and_sample_sensor1();
#endif
#if BOARD_CONF_USE_SENSOR2
    check_and_sample_sensor2();
#endif
#if BOARD_CONF_USE_SENSOR3
    check_and_sample_sensor3();
#endif
    return MANIKIN_STATUS_OK;
}