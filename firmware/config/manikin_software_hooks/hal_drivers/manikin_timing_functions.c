/**
 * @file             fake_timing_functions.c
 * @brief            Test stubs used in software tests for testing timing module
 *
 * @par
 * Copyright 2025 (C) RobotPatient Simulators
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of the Manikin Software Libraries V3 project
 *
 * Author:          Victor Hogeweij
 */
#include <stdint.h>
#include "manikin_timing_functions.h"
#include <manikin_software_conf.h>
#include "stm32f4xx_hal.h"

#define TIMER_FREQ_MHZ 168 // Tenths of MHz to avoid floats (16.8 MHz)
void DelayUS(uint32_t us) {
    uint32_t start = TIM5->CNT;
    uint32_t duration = (us * TIMER_FREQ_MHZ + 5) / 10;
    while (TIM5->CNT - start < duration);
}

int
manikin_hal_delay_us (size_t us)
{
    DelayUS(us);
    return 0;
}