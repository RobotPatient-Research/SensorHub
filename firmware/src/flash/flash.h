#ifndef FLASH_H
#define FLASH_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "lfs.h"
#include "stdint.h"
#include "stddef.h"
#include "common/manikin_types.h"
    /**
     * @brief This type exports the in the source-file initialized littlefs binding.
     */
    extern struct lfs_config lfs_flash_cfg;

    /**
     * @brief This function initializes the SPI flash driver with littlefs for wear-leveling and file-based access
     */
    void init_spi_flash_memory();

    manikin_status_t spi_flash_log_error(const char *msg, size_t len);

    size_t spi_flash_get_log_history(size_t max_buf_size, char *buffer);

    manikin_status_t spi_flash_clear_log(void);

    manikin_status_t spi_flash_set_sample_log_filename(const char *name);

    manikin_status_t spi_flash_on_new_sample (const uint8_t *cbor_buffer, const size_t len);

    manikin_status_t spi_flash_flush();
#ifdef __cplusplus
}
#endif
#endif /* FLASH_H */