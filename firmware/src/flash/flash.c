#include <spi/spi.h>
#include "lfs.h"
#include <w25qxx128/w25qxx128.h>
#include <stm32f4xx_hal.h>
#include "SEGGER_RTT.h"
#include "lwrb/lwrb.h"

// Define flash parameters
#define LFS_BLOCK_OFFSET 1024
#define W25QXX128_BLOCK_SIZE     4096 // Sector size
#define W25QXX128_BLOCK_COUNT    4096-LFS_BLOCK_OFFSET // Total number of 4K sectors in 128 Mbit
#define W25QXX128_READ_SIZE      256  // Minimum read size
#define W25QXX128_PROG_SIZE      256  // Minimum program size
#define W25QXX128_CACHE_SIZE     256
#define W25QXX128_LOOKAHEAD_SIZE 16

// --- RAW ASSERT LOG REGION ---
#define RAW_ASSERT_SECTOR_START (LFS_BLOCK_OFFSET - 4) // e.g., last 4 sectors before LFS region
#define RAW_ASSERT_SECTOR_COUNT 4
#define RAW_ASSERT_SECTOR_SIZE  4096
#define RAW_ASSERT_TOTAL_SIZE   (RAW_ASSERT_SECTOR_COUNT * RAW_ASSERT_SECTOR_SIZE)


#define SAMPLE_RAW_START_SECTOR  (LFS_BLOCK_OFFSET-128)
#define SAMPLE_RAW_SECTOR_COUNT  128  // e.g. 128 * 4KB = 512KB
#define SAMPLE_RAW_SECTOR_SIZE   4096
#define SAMPLE_RAW_TOTAL_SIZE    (SAMPLE_RAW_SECTOR_COUNT * SAMPLE_RAW_SECTOR_SIZE)

static uint32_t sample_ring_offset = 0;


static uint32_t raw_assert_offset = 0;  // tracks write head within raw region

// Log file name
#define DEFAULT_SAMPLE_FILE_NAME "man.txt"

uint8_t flash_read_buf[W25QXX128_CACHE_SIZE];
uint8_t flash_lookahead_buf[W25QXX128_LOOKAHEAD_SIZE];
uint8_t flash_prog_buf[W25QXX128_PROG_SIZE];

static char       sampling_file_name[32];
static lfs_file_t sampling_file;
static uint8_t    sampling_file_was_opened = false;

static size_t ring_offset;

manikin_spi_memory_ctx_t mem_ctx;

static int lfs_user_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);

static int lfs_user_prog(
    const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);

static int lfs_user_erase(const struct lfs_config *cfg, lfs_block_t block);

static int lfs_user_sync(const struct lfs_config *cfg);

lwrb_t flash_sample_buffer;
uint8_t flash_sample_buffer_data[2048];

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

    // int err = lfs_mount(&lfs, &lfs_flash_cfg);

    // // reformat if we can't mount the filesystem
    // // this should only happen on the first boot
    // if (err)
    // {
    //     lfs_format(&lfs, &lfs_flash_cfg);
    //     lfs_mount(&lfs, &lfs_flash_cfg);
    //  }
    lwrb_init(&flash_sample_buffer, flash_sample_buffer_data, sizeof(flash_sample_buffer_data));
    memcpy(sampling_file_name, DEFAULT_SAMPLE_FILE_NAME, sizeof(DEFAULT_SAMPLE_FILE_NAME));
// On boot:
sample_ring_offset = 0;
// Optionally: erase sample ring region once
for (uint32_t i = SAMPLE_RAW_START_SECTOR; i < SAMPLE_RAW_START_SECTOR + SAMPLE_RAW_SECTOR_COUNT; i++) {
    w25qxx_erase_sector(&mem_ctx, i);
}
}

int lfs_user_read (const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    block += LFS_BLOCK_OFFSET;  // SHIFT LOGICAL -> PHYSICAL
    uint32_t addr = block * cfg->block_size + off;
    if (w25qxx_read((manikin_spi_memory_ctx_t *)cfg->context, buffer, addr, size) != MANIKIN_MEMORY_RESULT_OK)
    {
        return LFS_ERR_IO;
    }
    return 0;
}

int lfs_user_prog (const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    block += LFS_BLOCK_OFFSET;  // SHIFT LOGICAL -> PHYSICAL
    uint32_t addr = block * cfg->block_size + off;
    SEGGER_RTT_printf(0, "PROG: logical block=%lu physical block=%lu off=%lu size=%lu addr=0x%lx\n",
                      block - LFS_BLOCK_OFFSET, block, off, size, addr);

    if ((addr % 256) + size > 256)
    {
        SEGGER_RTT_printf(0, "ERROR: write spans page boundary!\n");
        return LFS_ERR_IO;
    }

    if (w25qxx_write((manikin_spi_memory_ctx_t *)cfg->context, (uint8_t *)buffer, addr, size) != MANIKIN_MEMORY_RESULT_OK)
    {
        SEGGER_RTT_printf(0, "Write failed\n");
        return LFS_ERR_IO;
    }

    return 0;
}

int lfs_user_erase (const struct lfs_config *cfg, lfs_block_t block)
{
    block += LFS_BLOCK_OFFSET;  // SHIFT LOGICAL -> PHYSICAL
    SEGGER_RTT_printf(0, "ERASE: logical block=%lu physical block=%lu\n", block - LFS_BLOCK_OFFSET, block);
    if (w25qxx_erase_sector((manikin_spi_memory_ctx_t *)cfg->context, block) != MANIKIN_MEMORY_RESULT_OK)
    {
        SEGGER_RTT_printf(0, "Erase failed\n");
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

// Append an error message to the log file in LittleFS
manikin_status_t spi_flash_log_error(const char *msg, size_t len) {
    // Compute next write address in raw region
    if (raw_assert_offset + len > RAW_ASSERT_TOTAL_SIZE) {
        // Wrap around
        raw_assert_offset = 0;
    }

    uint32_t addr = RAW_ASSERT_SECTOR_START * RAW_ASSERT_SECTOR_SIZE + raw_assert_offset;

    manikin_memory_result_t res = w25qxx_write(&mem_ctx, (uint8_t*)msg, addr, len);
    if (res != MANIKIN_MEMORY_RESULT_OK) {
        SEGGER_RTT_printf(0, "RAW ASSERT WRITE FAILED: %d\n", res);
        return MANIKIN_STATUS_ERR_WRITE_FAIL;
    }

    // Optionally add newline if messages are ASCII
    const char newline = '\n';
    if (raw_assert_offset + len + 1 <= RAW_ASSERT_TOTAL_SIZE) {
        w25qxx_write(&mem_ctx, (uint8_t*)&newline, addr + len, 1);
        raw_assert_offset += 1;
    }

    raw_assert_offset += len;

    SEGGER_RTT_printf(0, "RAW ASSERT WRITTEN at 0x%lx len=%lu\n", addr, len);
    return MANIKIN_STATUS_OK;
}


// Read log history into user buffer
size_t
spi_flash_get_log_history (size_t max_buf_size, char *buffer)
{
    uint32_t addr = RAW_ASSERT_SECTOR_START * RAW_ASSERT_SECTOR_SIZE;
    size_t to_read = RAW_ASSERT_TOTAL_SIZE < max_buf_size ? RAW_ASSERT_TOTAL_SIZE : max_buf_size;

    if (w25qxx_read(&mem_ctx, (uint8_t*)buffer, addr, to_read) != MANIKIN_MEMORY_RESULT_OK) {
        SEGGER_RTT_printf(0, "RAW ASSERT READ FAILED\n");
        return 0;
    }
    return to_read;
}

// Clear the log file by truncating it to zero length
manikin_status_t
spi_flash_clear_log (void)
{
// Erase raw assert log region once if desired (optional)
for (uint32_t i = RAW_ASSERT_SECTOR_START; i < RAW_ASSERT_SECTOR_START + RAW_ASSERT_SECTOR_COUNT; i++) {
    w25qxx_erase_sector(&mem_ctx, i);
    HAL_Delay(50);
}
// Could also scan to recover last offset if desired — simple version starts at 0
raw_assert_offset = 0;

    return MANIKIN_STATUS_OK;
}

manikin_status_t
spi_flash_set_sample_log_filename (const char *name)
{
    if (sampling_file_was_opened)
    {
        lfs_file_close(&lfs, &sampling_file);
        memcpy(sampling_file_name, DEFAULT_SAMPLE_FILE_NAME, sizeof(DEFAULT_SAMPLE_FILE_NAME));
        sampling_file_was_opened = false;
    }
    memcpy(sampling_file_name, name, strlen(name));
    return MANIKIN_STATUS_OK;
}

manikin_status_t 
spi_flash_on_new_sample (const uint8_t *cbor_buffer, const size_t len)
{
    lwrb_write(&flash_sample_buffer, cbor_buffer, len);
    return MANIKIN_STATUS_OK;
}

char temp_buf[256];
manikin_status_t
spi_flash_flush ()
{
    size_t len = lwrb_get_full(&flash_sample_buffer);
    if (len < 200) {
        return MANIKIN_STATUS_OK;
    }

    len = lwrb_read(&flash_sample_buffer, temp_buf, sizeof(temp_buf));

    if (sample_ring_offset + len >= SAMPLE_RAW_TOTAL_SIZE) {
        sample_ring_offset = 0; // wrap
    }

    uint32_t addr = SAMPLE_RAW_START_SECTOR * SAMPLE_RAW_SECTOR_SIZE + sample_ring_offset;

    if (w25qxx_write(&mem_ctx, (uint8_t*)temp_buf, addr, len) != MANIKIN_MEMORY_RESULT_OK) {
        SEGGER_RTT_printf(0, "RAW SAMPLE WRITE FAILED!\n");
        return MANIKIN_STATUS_ERR_WRITE_FAIL;
    }

    sample_ring_offset += len;

    return MANIKIN_STATUS_OK;
}