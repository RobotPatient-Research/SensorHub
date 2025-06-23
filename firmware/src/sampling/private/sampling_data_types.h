#ifndef SAMPLING_DATA_TYPES_H
#define SAMPLING_DATA_TYPES_H
#include "board_conf.h"

#if BOARD_CONF_USE_SENSOR1
/* CAN data packaging */
typedef struct
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR1_SAMPLE_T data;
} sample_sensor1_t;
#endif

#if BOARD_CONF_USE_SENSOR2
typedef struct
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR2_SAMPLE_T data;
} sample_sensor2_t;
#endif

#if BOARD_CONF_USE_SENSOR3
typedef struct
{
    char                        sensor_name[8];
    uint32_t                    frame_id;
    BOARD_CONF_SENSOR3_SAMPLE_T data;
} sample_sensor3_t;
#endif
#endif /* SAMPLING_DATA_TYPES_H */