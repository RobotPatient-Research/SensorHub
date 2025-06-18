#include "manikin_spi_functions.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
SPI_HandleTypeDef spi1_handle;
int
spi_hal_init (manikin_spi_inst_t spi_inst, uint32_t baud)
{
    static DMA_HandleTypeDef hdma_tx;
    static DMA_HandleTypeDef hdma_rx;
    /*##-3- Configure the DMA streams
     * ##########################################*/
    /* Configure the DMA handler for Transmission process */
    hdma_tx.Instance = DMA1_Stream5;

    hdma_tx.Init.Channel             = DMA_CHANNEL_0;
    hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_tx.Init.Mode                = DMA_NORMAL;
    hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
    hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
    hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;

    HAL_DMA_Init(&hdma_tx);

    /* Associate the initialized DMA handle to the the SPI handle */
    __HAL_LINKDMA(&spi1_handle, hdmatx, hdma_tx);

    /* Configure the DMA handler for Transmission process */
    hdma_rx.Instance = DMA1_Stream0;

    hdma_rx.Init.Channel             = DMA_CHANNEL_0;
    hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_rx.Init.Mode                = DMA_NORMAL;
    hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
    hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;

    HAL_DMA_Init(&hdma_rx);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    /* NVIC configuration for DMA transfer complete interrupt (SPI3_RX) */
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    /*##-5- Configure the NVIC for SPI
     * #########################################*/
    HAL_NVIC_SetPriority(SPI3_IRQn, 0, 2);
    HAL_NVIC_EnableIRQ(SPI3_IRQn);
    /* Associate the initialized DMA handle to the the SPI handle */
    __HAL_LINKDMA(&spi1_handle, hdmarx, hdma_rx);
    spi1_handle.Instance               = spi_inst;
    spi1_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    spi1_handle.Init.Direction         = SPI_DIRECTION_2LINES;
    spi1_handle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    spi1_handle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    spi1_handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    spi1_handle.Init.CRCPolynomial     = 7;
    spi1_handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    spi1_handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    spi1_handle.Init.NSS               = SPI_NSS_SOFT;
    spi1_handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    spi1_handle.Init.Mode              = SPI_MODE_MASTER;
    return HAL_SPI_Init(&spi1_handle) == HAL_OK ? 0 : 9;
}

int
spi_hal_set_cs (manikin_spi_cs_t spi_cs, uint8_t state)
{
    HAL_GPIO_WritePin(spi_cs.port, spi_cs.pin, state);
    return 0;
}

size_t
spi_hal_write_bytes (manikin_spi_inst_t spi_inst, uint8_t *data, size_t len)
{
    while (HAL_SPI_GetState(&spi1_handle) != HAL_SPI_STATE_READY)
    {
    }
    uint32_t status = HAL_SPI_Transmit_DMA(&spi1_handle, (uint8_t *)data, len);
    while (HAL_SPI_GetState(&spi1_handle) != HAL_SPI_STATE_READY)
    {
    }
    return status == HAL_OK ? len : 0;
}

size_t
spi_hal_read_bytes (manikin_spi_inst_t spi_inst, uint8_t *data, size_t len)
{
    while (HAL_SPI_GetState(&spi1_handle) != HAL_SPI_STATE_READY)
    {
    }
    uint32_t status = HAL_SPI_Receive_DMA(&spi1_handle, (uint8_t *)data, len);
    while (HAL_SPI_GetState(&spi1_handle) != HAL_SPI_STATE_READY)
    {
    }
    return status == HAL_OK ? len : 0;
}

int
spi_hal_deinit (manikin_spi_inst_t spi_inst)
{
    return HAL_SPI_DeInit(&spi1_handle) == HAL_OK ? 0 : 10;
}