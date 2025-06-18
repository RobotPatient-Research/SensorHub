#ifndef FLASH_H
#define FLASH_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "lfs.h"

    extern struct lfs_config lfs_flash_cfg;
    void                     init_spi_flash_memory();

#ifdef __cplusplus
}
#endif
#endif /* FLASH_H */