#ifndef HAL_MSP_H
#define HAL_MSP_H

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief System Clock Configuration
     * @retval None
     */
    void SystemClock_Config(void);

    /**
     * @brief GPIO Initialization Function
     * @param None
     * @retval None
     */
    void BOARD_GPIO_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_MSP_H */