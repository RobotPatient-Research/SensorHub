#ifndef FLASH_H
#define FLASH_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "lfs.h"

    /**
     * @brief This type exports the in the source-file initialized littlefs binding.
     */
    extern struct lfs_config lfs_flash_cfg;

    /**
     * @brief This function initializes the SPI flash driver with littlefs for wear-leveling and file-based access
     */
    void init_spi_flash_memory();

#ifdef __cplusplus
}
#endif
#endif /* FLASH_H */