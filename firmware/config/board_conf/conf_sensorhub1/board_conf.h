#ifndef BOARD_CONF_H
#define BOARD_CONF_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f405xx.h"
/* Sensor includes */
#include "vl6180x/vl6180x.h"
#include "ads7138/ads7138.h"
#include "sdp810/sdp810.h"

#define BOARD_CONF_SENSORHUB_ID 1U

/**
 * CAN Configuration
 */
#define BOARD_CONF_CAN_SPEED    500000
#define BOARD_CONF_CAN_INSTANCE CAN1

#define BOARD_CONF_CAN_GPIO_CLK_EN() __HAL_RCC_GPIOB_CLK_ENABLE()
#define BOARD_CONF_CAN_RX_PIN_MUX GPIO_AF9_CAN1
#define BOARD_CONF_CAN_RX_PORT GPIOB
#define BOARD_CONF_CAN_RX_PIN  GPIO_PIN_8

#define BOARD_CONF_CAN_TX_PIN_MUX GPIO_AF9_CAN1
#define BOARD_CONF_CAN_TX_PORT GPIOB
#define BOARD_CONF_CAN_TX_PIN  GPIO_PIN_9

#define BOARD_CONF_CAN_CLK_EN() __HAL_RCC_CAN1_CLK_ENABLE()

#define BOARD_CONF_CAN_SENSOR1_TX_ID  0x080
#define BOARD_CONF_CAN_SENSOR1_RX_ID  0x180U
#define BOARD_CONF_CAN_SENSOR2_TX_ID  0x01U
#define BOARD_CONF_CAN_SENSOR2_RX_ID  0x101U
#define BOARD_CONF_CAN_SENSOR3_TX_ID  0x050U
#define BOARD_CONF_CAN_SENSOR3_RX_ID  0x150U

#define BOARD_CONF_CAN_STATUS_TX_ID  0x10U
#define BOARD_CONF_CAN_STATUS_RX_ID  0x201U

#define BOARD_CONF_CAN_GLOBAL_BRDCAST_RX_ID  0x0U
/**
 * I2C Configuration
 */
#define BOARD_CONF_I2C0_SPEED    400000
#define BOARD_CONF_I2C0_INSTANCE I2C2

#define BOARD_CONF_I2C0_CLK_EN()    __HAL_RCC_I2C2_CLK_ENABLE()
#define BOARD_CONF_I2C0_SDA_PIN_MUX GPIO_AF4_I2C2
#define BOARD_CONF_I2C0_SDA_PORT    GPIOB
#define BOARD_CONF_I2C0_SDA_PIN     GPIO_PIN_11

#define BOARD_CONF_I2C0_SCL_PIN_MUX GPIO_AF4_I2C2
#define BOARD_CONF_I2C0_SCL_PORT    GPIOB
#define BOARD_CONF_I2C0_SCL_PIN     GPIO_PIN_10


/**
* Led Configuration
*/
#define BOARD_CONF_LED_PORT GPIOA
#define BOARD_CONF_LED_PIN GPIO_PIN_15
#define BOARD_CONF_LED_PORT_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()

/**************************************/

#define BOARD_CONF_I2C1_SPEED    400000
#define BOARD_CONF_I2C1_INSTANCE I2C1

#define BOARD_CONF_I2C1_CLK_EN()    __HAL_RCC_I2C1_CLK_ENABLE()
#define BOARD_CONF_I2C1_SDA_PIN_MUX GPIO_AF4_I2C1
#define BOARD_CONF_I2C1_SDA_PORT    GPIOB
#define BOARD_CONF_I2C1_SDA_PIN     GPIO_PIN_7

#define BOARD_CONF_I2C1_SCL_PIN_MUX GPIO_AF4_I2C1
#define BOARD_CONF_I2C1_SCL_PORT    GPIOB
#define BOARD_CONF_I2C1_SCL_PIN     GPIO_PIN_6

/**
 *  Timer Configuration
 */
#define BOARD_CONF_TIMER_SENSOR_1_EN() __HAL_RCC_TIM2_CLK_ENABLE()
#define BOARD_CONF_TIMER_SENSOR_1      TIM2

#define BOARD_CONF_TIMER_SENSOR_2_EN() __HAL_RCC_TIM3_CLK_ENABLE()
#define BOARD_CONF_TIMER_SENSOR_2      TIM3

#define BOARD_CONF_TIMER_SENSOR_3_EN() __HAL_RCC_TIM4_CLK_ENABLE()
#define BOARD_CONF_TIMER_SENSOR_3      TIM4

/**************************************/
/*    Sensor & Flash configuration    */
/**************************************/

#define BOARD_CONF_USE_SENSOR1 1
#define BOARD_CONF_USE_SENSOR2 1
#define BOARD_CONF_USE_SENSOR3 1


/**
 * Sensor 1 configuration
 */
#if BOARD_CONF_USE_SENSOR1
#define BOARD_CONF_SENSOR1_NAME             "vl6180"
#define BOARD_CONF_SENSOR1_RESET_PORT       GPIOA
#define BOARD_CONF_SENSOR1_RESET_PIN        GPIO_PIN_9
#define BOARD_CONF_SENSOR1_ADDR             0x29
#define BOARD_CONF_SENSOR1_SAMPLE_RATE_HZ   100
#define BOARD_CONF_SENSOR1_INIT(sensor_ctx) vl6180x_init_sensor(sensor_ctx)
#define BOARD_CONF_SENSOR1_SAMPLE(sensor_ctx, read_buf) \
    vl6180x_read_sensor(sensor_ctx, read_buf)
#define BOARD_CONF_SENSOR1_DEINIT(sensor_ctx) vl6180x_deinit_sensor(sensor_ctx)
#define BOARD_CONF_SENSOR1_SAMPLE_T vl6180x_sample_data_t
#define BOARD_CONF_SENSOR1_SAMPLE_PARSE(read_buf, sample_data) vl6180x_parse_raw_data(read_buf, sample_data)
#endif // BOARD_CONF_USE_SENSOR1

/**
 * Sensor 2 configuration
 */
#if BOARD_CONF_USE_SENSOR2
#define BOARD_CONF_SENSOR2_NAME             "ads7138"
#define BOARD_CONF_SENSOR2_RESET_PORT       GPIOA
#define BOARD_CONF_SENSOR2_RESET_PIN        GPIO_PIN_10
#define BOARD_CONF_SENSOR2_ADDR             0x10
#define BOARD_CONF_SENSOR2_SAMPLE_RATE_HZ   50
#define BOARD_CONF_SENSOR2_INIT(sensor_ctx) ads7138_init_sensor(sensor_ctx)
#define BOARD_CONF_SENSOR2_SAMPLE(sensor_ctx, read_buf) \
    ads7138_read_sensor(sensor_ctx, read_buf)
#define BOARD_CONF_SENSOR2_DEINIT(sensor_ctx) ads7138_deinit_sensor(sensor_ctx)
#define BOARD_CONF_SENSOR2_SAMPLE_T ads7138_sample_data_t
#define BOARD_CONF_SENSOR2_SAMPLE_PARSE(read_buf, sample_data) ads7138_parse_raw_data(read_buf, sample_data)
#endif // BOARD_CONF_USE_SENSOR2

/**
 * Sensor 3 configuration
 */
 #if BOARD_CONF_USE_SENSOR3
 #define BOARD_CONF_SENSOR3_NAME             "SDP810"
 #define BOARD_CONF_SENSOR3_ADDR             0x25
 #define BOARD_CONF_SENSOR3_SAMPLE_RATE_HZ   50
 #define BOARD_CONF_SENSOR3_INIT(sensor_ctx) sdp810_init_sensor(sensor_ctx)
 #define BOARD_CONF_SENSOR3_SAMPLE(sensor_ctx, read_buf) \
     sdp810_read_sensor(sensor_ctx, read_buf)
 #define BOARD_CONF_SENSOR3_DEINIT(sensor_ctx) sdp810_deinit_sensor(sensor_ctx)
 #define BOARD_CONF_SENSOR3_SAMPLE_T sdp810_sample_data_t
 #define BOARD_CONF_SENSOR3_SAMPLE_PARSE(read_buf, sample_data) sdp810_parse_raw_data(read_buf, sample_data)
 #endif // BOARD_CONF_USE_SENSOR3

/**
 * FLASH Configuration
 */
 #ifdef __cplusplus
}
#endif
#endif /* BOARD_CONF_H */