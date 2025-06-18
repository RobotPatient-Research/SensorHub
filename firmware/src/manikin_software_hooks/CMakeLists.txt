add_library(manikin_software_hooks hal_drivers/manikin_i2c_functions.c
                                   hal_drivers/manikin_timer_functions.c
                                   hal_drivers/manikin_watchdog_functions.c
                                   hal_drivers/manikin_spi_functions.c
                                   hal_drivers/manikin_error_functions.c)

target_link_libraries(manikin_software_hooks CMSIS::STM32::F405RG HAL::STM32::F4::CORTEX HAL::STM32::F4::TIM HAL::STM32::F4::I2C HAL::STM32::F4::WWDG HAL::STM32::F4::TIMEx HAL::STM32::F4::DMA  HAL::STM32::F4::SPI)
target_include_directories(manikin_software_hooks PUBLIC ${CMAKE_CURRENT_LIST_DIR}/conf ${CMAKE_CURRENT_LIST_DIR}/hal_drivers)
