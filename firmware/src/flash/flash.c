#include <spi/spi.h>
#include "lfs.h"
#include <w25qxx128/w25qxx128.h>
#include <stm32f4xx_hal.h>
#include "SEGGER_RTT.h"

// Define flash parameters
#define W25QXX128_BLOCK_SIZE     4096 // Sector size
#define W25QXX128_BLOCK_COUNT    4096 // Total number of 4K sectors in 128 Mbit
#define W25QXX128_READ_SIZE      256  // Minimum read size
#define W25QXX128_PROG_SIZE      256  // Minimum program size
#define W25QXX128_CACHE_SIZE     256
#define W25QXX128_LOOKAHEAD_SIZE 16

uint8_t flash_read_buf[W25QXX128_CACHE_SIZE];
uint8_t flash_lookahead_buf[W25QXX128_LOOKAHEAD_SIZE];
uint8_t flash_prog_buf[W25QXX128_PROG_SIZE];

manikin_spi_memory_ctx_t mem_ctx;

static int lfs_user_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);

static int lfs_user_prog(
    const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);

static int lfs_user_erase(const struct lfs_config *cfg, lfs_block_t block);

static int lfs_user_sync(const struct lfs_config *cfg);

// variables used by the filesystem
lfs_t             lfs;
struct lfs_config lfs_flash_cfg = {
    .context = &mem_ctx, // This should be initialized with actual SPI and chip config
    .read    = lfs_user_read,
    .prog    = lfs_user_prog,
    .erase   = lfs_user_erase,
    .sync    = lfs_user_sync,

    .read_size        = W25QXX128_READ_SIZE,
    .prog_size        = W25QXX128_PROG_SIZE,
    .block_size       = W25QXX128_BLOCK_SIZE,
    .block_count      = W25QXX128_BLOCK_COUNT,
    .cache_size       = W25QXX128_CACHE_SIZE,
    .lookahead_size   = W25QXX128_LOOKAHEAD_SIZE,
    .block_cycles     = 500, // Adjust based on expected endurance
    .read_buffer      = flash_read_buf,
    .prog_buffer      = flash_prog_buf,
    .lookahead_buffer = flash_lookahead_buf,
};

void
init_spi_flash_memory ()
{
    GPIO_InitTypeDef spi_gpio;
    spi_gpio.Pin       = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    spi_gpio.Mode      = GPIO_MODE_AF_PP;
    spi_gpio.Pull      = GPIO_PULLUP;
    spi_gpio.Alternate = GPIO_AF6_SPI3;
    spi_gpio.Speed     = GPIO_SPEED_FAST;
    HAL_GPIO_Init(GPIOC, &spi_gpio);
    spi_gpio.Pin  = GPIO_PIN_3;
    spi_gpio.Pull = GPIO_NOPULL;
    spi_gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOC, &spi_gpio);

    manikin_spi_cs_t cs_pin = { .port = GPIOC, .pin = GPIO_PIN_3 };
    mem_ctx.mem_size        = 16;
    mem_ctx.spi             = SPI3;
    mem_ctx.spi_cs          = cs_pin;
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_SPI3_CLK_ENABLE();
    manikin_spi_init(SPI3, (10 * 10e6));
    w25qxx_init(&mem_ctx);
    int err = lfs_mount(&lfs, &lfs_flash_cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err)
    {
        lfs_format(&lfs, &lfs_flash_cfg);
        lfs_mount(&lfs, &lfs_flash_cfg);
    }
}

int
lfs_user_read (const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t addr = block * cfg->block_size + off;
    if (w25qxx_read((manikin_spi_memory_ctx_t *)cfg->context, buffer, addr, size) != MANIKIN_MEMORY_RESULT_OK)
    {
        return LFS_ERR_IO;
    }
    return 0;
}

int
lfs_user_prog (const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t addr = block * cfg->block_size + off;
    SEGGER_RTT_printf(0, "PROG: block=%lu off=%lu size=%lu addr=0x%lx\n", block, off, size, addr);

    if ((addr % 256) + size > 256)
    {
        SEGGER_RTT_printf(0, "ERROR: write spans page boundary!\n");
        return LFS_ERR_IO;
    }

    manikin_memory_result_t res = w25qxx_write((manikin_spi_memory_ctx_t *)cfg->context, (uint8_t *)buffer, addr, size);
    if (res != MANIKIN_MEMORY_RESULT_OK)
    {
        SEGGER_RTT_printf(0, "Write failed with code %d\n", res);
        return LFS_ERR_IO;
    }

    return 0;
}

int
lfs_user_erase (const struct lfs_config *cfg, lfs_block_t block)
{
    printf("ERASE: block=%lu\n", block);
    manikin_memory_result_t res = w25qxx_erase_sector((manikin_spi_memory_ctx_t *)cfg->context, block);
    if (res != MANIKIN_MEMORY_RESULT_OK)
    {
        printf("Erase failed with code %d\n", res);
        return LFS_ERR_IO;
    }
    return 0;
}

int
lfs_user_sync (const struct lfs_config *cfg)
{
    // No-op for many SPI NOR devices, since write operations are already
    // synchronized

    return 0;
}
