#ifndef MANIKIN_CONF_H
#define MANIKIN_CONF_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stm32f4xx.h>
#include <stdint.h>

#define MANIKIN_SOFTWARE_MAX_TIMEOUT   1u
#define MANIKIN_FLASH_MAX_RETRIES      3u
#define MANIKIN_SOFTWARE_MIN_SPI_SPEED (10u * 10e6)
#define MANIKIN_SOFTWARE_MAX_SPI_SPEED (50u * 10e6)

    typedef SPI_TypeDef *manikin_spi_inst_t;

    typedef I2C_TypeDef *manikin_i2c_inst_t;

    typedef TIM_TypeDef *manikin_timer_inst_t;

    typedef WWDG_TypeDef *manikin_watchdog_inst_t;

    typedef struct
    {
        GPIO_TypeDef *port;
        uint8_t       pin;
    } manikin_spi_cs_t;

#ifdef __cplusplus
}
#endif
#endif /* MANIKIN_CONF_H */