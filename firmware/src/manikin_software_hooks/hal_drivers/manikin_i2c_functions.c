#include "manikin_i2c_functions.h"
#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"

static I2C_HandleTypeDef handle;
extern volatile uint8_t  trigger_i2c;

int
i2c_hal_init (manikin_i2c_inst_t i2c_inst, uint32_t baud)
{
    handle.Instance             = i2c_inst;
    handle.Init.ClockSpeed      = baud;
    handle.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    handle.Init.OwnAddress1     = 0;
    handle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    handle.Init.OwnAddress2     = 0;
    handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    handle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    return HAL_I2C_Init(&handle) != HAL_OK;
}

uint8_t
i2c_hal_device_acknowledge (manikin_i2c_inst_t i2c_inst, uint8_t i2c_addr)
{
    return 0;
}

uint8_t
i2c_hal_full_bus_hang (GPIO_TypeDef *port, uint16_t scl_pin, uint16_t sda_pin)
{
    uint32_t scl = ((port->IDR) & scl_pin);
    uint32_t sda = ((port->IDR) & sda_pin);
    return (scl == 0 && sda == 0);
}

uint8_t
i2c_hal_slave_hang (GPIO_TypeDef *port, uint16_t sda_pin)
{
    uint32_t sda = ((port->IDR) & sda_pin);
    return (sda == 0);
}

void
i2c_gpio_bitbang_recover (GPIO_TypeDef *port,
                          uint16_t      scl_pin,
                          uint16_t      sda_pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin              = scl_pin | sda_pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull             = GPIO_NOPULL;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    for (int i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
        for (volatile int i = 0; i < 1000; i++)
        {
            __NOP(); // Approx 10-50us
        }

        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
        for (volatile int i = 0; i < 1000; i++)
        {
            __NOP(); // Approx 10-50us
        }
    }

    // 4. Generate a stop condition: SDA low -> high while SCL is high
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
    for (volatile int i = 0; i < 1000; i++)
    {
        __NOP(); // Approx 10-50us
    }

    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_SET);
    for (volatile int i = 0; i < 1000; i++)
    {
        __NOP(); // Approx 10-50us
    }

    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // Adjust based on your config
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

int
i2c_hal_error_flag_check (manikin_i2c_inst_t i2c_inst)
{
    if (i2c_inst == I2C2)
    {
        if (i2c_hal_full_bus_hang(GPIOB, GPIO_PIN_11, GPIO_PIN_10))
        {
            /* Do full reset*/
            return 1;
        }
        else if (i2c_hal_slave_hang(GPIOB, GPIO_PIN_11))
        {
            /* Try manual reset */
            i2c_gpio_bitbang_recover(GPIOB, GPIO_PIN_11, GPIO_PIN_10);
            if (i2c_hal_slave_hang(GPIOB, GPIO_PIN_11))
            {
                return 0;
            }
        }
    }
    else if (i2c_inst == I2C1)
    {
        if (i2c_hal_full_bus_hang(GPIOB, GPIO_PIN_7, GPIO_PIN_6))
        {
            /* Do full reset*/
            return 1;
        }
        else if (i2c_hal_slave_hang(GPIOB, GPIO_PIN_7))
        {
            /* Try manual reset */
            i2c_gpio_bitbang_recover(GPIOB, GPIO_PIN_7, GPIO_PIN_6);
            if (i2c_hal_slave_hang(GPIOB, GPIO_PIN_7))
            {
                return 0;
            }
        }
    }
    return 0;
}

size_t
i2c_hal_read_bytes (manikin_i2c_inst_t i2c_inst,
                    uint8_t            i2c_addr,
                    uint8_t           *data,
                    size_t             len)
{
    handle.Instance = i2c_inst;
    handle.State    = HAL_I2C_STATE_READY;
    if (HAL_I2C_Master_Receive(&handle, i2c_addr, data, len, 1) != HAL_OK)
    {
        return 0;
    }
    return len;
}

size_t
i2c_hal_write_bytes (manikin_i2c_inst_t i2c_inst,
                     uint8_t            i2c_addr,
                     const uint8_t     *data,
                     size_t             len)
{

    handle.Instance = i2c_inst;
    handle.State    = HAL_I2C_STATE_READY;
    if (HAL_I2C_Master_Transmit(&handle, i2c_addr, (uint8_t *)data, len, 1)
        != HAL_OK)
    {
        return 0;
    }
    return len;
}

int
i2c_hal_deinit (manikin_i2c_inst_t i2c_inst)
{
    handle.Instance = i2c_inst;
    return HAL_I2C_DeInit(&handle) != HAL_OK;
}

int
i2c_hal_bus_reset (manikin_i2c_inst_t i2c_inst)
{
    /* Clear i2c reg */
    i2c_inst->DR = 0;
    /* Remove power from the sensors */
    if (i2c_inst == I2C2)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 0);
    }

    return 0;
}

uint32_t
i2c_hal_get_baud (manikin_i2c_inst_t i2c_inst)
{
    uint32_t raw_baud
        = ((i2c_inst->CR2 & I2C_CR2_FREQ_Msk) >> I2C_CR2_FREQ_Pos) * 1000000U;
    if (raw_baud > 35000000 && raw_baud < 45000000)
    {
        return 400000;
    }
    else if (raw_baud > 5000000 && raw_baud < 15000000)
    {
        return 100000;
    }
    return 100000;
}

int
i2c_hal_bus_recover (manikin_i2c_inst_t i2c_inst)
{
    /* Enable power to the sensors */
    if (i2c_inst == I2C2)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1);
    }
    else
    {

        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1);
    }

    return 0;
}