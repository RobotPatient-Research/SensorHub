#include "cli.h"
#include "cbor/base.h"
#include "cbor/encoder.h"

#include "usbd_cdc_if.h"
#include "lwrb/lwrb.h"

#include <string.h>

/* Local ring buffer for CLI input */
static lwrb_t  cli_buffer;
static uint8_t cli_buffer_data[512];
static uint8_t packet_data[512];

/* CBOR writer context */
static cbor_writer_t writer;

/* Handle input from CDC (USB serial) */
void
manikin_cli_on_input (const uint8_t *buf, const uint32_t len)
{
    if ((len == 1U) && (buf[0] == '\r'))
    {
        size_t num_bytes = lwrb_read(&cli_buffer, packet_data, sizeof(packet_data));
        (void)CDC_Transmit_FS(packet_data, num_bytes);
    }
    else
    {
        (void)lwrb_write(&cli_buffer, buf, len);
    }
}

/* Format new sensor sample into CBOR and return length */
size_t
manikin_cli_on_new_sensor_sample (const char    *sample_name,
                                  const size_t   sample_id,
                                  const uint8_t *buffer,
                                  const size_t   len)
{
    if ((sample_name == NULL) || (buffer == NULL) || len > sizeof(packet_data))
    {
        return 0U;
    }

    cbor_writer_init(&writer, packet_data, sizeof(packet_data));
    cbor_encode_map(&writer, 3);

    cbor_encode_null_terminated_text_string(&writer, "data");
    cbor_encode_array(&writer, len);
    for (size_t i = 0U; i < len; ++i)
    {
        cbor_encode_unsigned_integer(&writer, buffer[i]);
    }

    cbor_encode_null_terminated_text_string(&writer, "sensor");
    cbor_encode_null_terminated_text_string(&writer, sample_name);

    cbor_encode_null_terminated_text_string(&writer, "id");
    cbor_encode_unsigned_integer(&writer, sample_id);

    const size_t encoded_len = cbor_writer_len(&writer);

    packet_data[encoded_len]     = '\r';
    packet_data[encoded_len + 1] = '\n';

    lwrb_write(&cli_buffer, packet_data, encoded_len + 2U);
    return encoded_len + 2U;
}

/* Initialize CLI interface */
void
manikin_cli_init (void)
{
    lwrb_init(&cli_buffer, cli_buffer_data, sizeof(cli_buffer_data));
}

/* Send data over USB CDC */
manikin_status_t
print_to_stdout (void)
{

    return MANIKIN_STATUS_OK;
}

manikin_status_t
manikin_cli_flush ()
{
    const uint32_t len = lwrb_read(&cli_buffer, cli_buffer_data, sizeof(cli_buffer_data));
    (void)CDC_Transmit_FS(cli_buffer_data, len);
    return MANIKIN_STATUS_OK;
}