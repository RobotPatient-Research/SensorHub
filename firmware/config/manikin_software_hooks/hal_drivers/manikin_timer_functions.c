#include <manikin_timer_functions.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_tim.h>
#include <system_stm32f4xx.h>

static TIM_HandleTypeDef timer2_handle;
static TIM_HandleTypeDef timer3_handle;
static TIM_HandleTypeDef timer4_handle;

void
compute_freq (TIM_HandleTypeDef *tim, int wanted_freq)
{
    uint32_t TIM_CLK   = 168000000; // 168 MHz
    uint32_t prescaler = 0;
    uint32_t period    = 0;

    for (prescaler = 0; prescaler < 0xFFFF; prescaler++)
    {
        uint32_t tmp_period = TIM_CLK / ((prescaler + 1) * wanted_freq);
        if (tmp_period > 0)
        {
            tmp_period -= 1;
        }

        if (tmp_period <= 0xFFFF)
        {
            period = tmp_period;
            break;
        }
    }

    // Assign values to timer init structure
    tim->Init.Prescaler = prescaler;
    tim->Init.Period    = period;
}

int
timer_hal_init (manikin_timer_inst_t timer_inst, uint32_t freq)
{
    TIM_HandleTypeDef *timer_handle;
    if (timer_inst == TIM3)
    {
        timer_handle = &timer3_handle;
    }
    else if (timer_inst == TIM2)
    {
        timer_handle = &timer2_handle;
    }
    else
    {
        timer_handle = &timer4_handle;
    }

    compute_freq(timer_handle, 2 * freq);
    timer_handle->Instance               = timer_inst;
    timer_handle->Init.RepetitionCounter = 0;
    timer_handle->Init.ClockDivision     = 0;
    timer_handle->Init.CounterMode       = TIM_COUNTERMODE_UP;
    timer_handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (timer_inst == TIM2)
    {
        NVIC_EnableIRQ(TIM2_IRQn);
        NVIC_SetPriority(TIM2_IRQn, 0);
    }
    else if (timer_inst == TIM3)
    {
        NVIC_EnableIRQ(TIM3_IRQn);
        NVIC_SetPriority(TIM3_IRQn, 0);
    }
    else
    {
        NVIC_EnableIRQ(TIM4_IRQn);
        NVIC_SetPriority(TIM4_IRQn, 0);
    }
    if (HAL_TIM_Base_Init(timer_handle) != HAL_OK)
    {
        return 1;
    }
    return 0;
}

int
timer_hal_start (manikin_timer_inst_t timer_inst)
{
    TIM_HandleTypeDef *timer_handle;
    if (timer_inst == TIM3)
    {
        timer_handle = &timer3_handle;
    }
    else if (timer_inst == TIM2)
    {
        timer_handle = &timer2_handle;
    }
    else
    {
        timer_handle = &timer4_handle;
    }
    timer_inst->DIER |= TIM_DIER_UIE;
    timer_inst->SR |= TIM_CR1_CEN;
    HAL_TIM_Base_Start(timer_handle);
    return 0;
}

int
timer_hal_stop (manikin_timer_inst_t timer_inst)
{
    TIM_HandleTypeDef *timer_handle;
    if (timer_inst == TIM3)
    {
        timer_handle = &timer3_handle;
    }
    else if (timer_inst == TIM2)
    {
        timer_handle = &timer2_handle;
    }
    else
    {
        timer_handle = &timer4_handle;
    }
    timer_handle->Instance = timer_inst;
    timer_inst->SR &= ~TIM_CR1_CEN;
    HAL_TIM_Base_Stop(timer_handle);
    return 0;
}

int
timer_hal_deinit (manikin_timer_inst_t timer_inst)
{
    return 0;
}

size_t
timer_hal_get_tick ()
{
    return 0;
}