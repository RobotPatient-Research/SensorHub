#include "manikin_error_functions.h"
#include "manikin_software_conf.h"
#include "flash.h"

#define UNUSED(X) (void)X /* To avoid gcc/g++ warnings */
static char log_buffer[512];

void
critical_error (uint32_t hash, uint32_t line)
{
    size_t len = snprintf(log_buffer, sizeof(log_buffer), "File: %ld, Line: %ld\r\n", hash, line);
    spi_flash_log_error(log_buffer, len);
}

void
non_critical_error (uint32_t hash, uint32_t line)
{
    size_t len = snprintf(log_buffer, sizeof(log_buffer), "File: %ld, Line: %ld\r\n", hash, line);
    spi_flash_log_error(log_buffer, len);
}