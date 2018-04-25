/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Test: CPU stress + Memory usage + IO Access
 * Activity:
 *    GPS position generator
 *    Speed and direction computation (+ log)
 *    Altitude and pression computation (+ log)
 *
 * The aim of the test is to share the resources between partitions.
 * We excpect these tests to allow us to analyse the memory access management
 * the RTOS implemented and how it reacts with memory and IO acces
 * intensive applications.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libm.h>
#include <types.h>
#include <float.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>
#include <arinc653/semaphore.h>
#include <arinc653/queueing.h>

#include "../../config.h"

//#define GET_STAT 1
#include "../../../BENCH_TOOLS/get_stat.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

#define GENERATOR_LOAD 10000  /* Processing simulation */
//#define VERBOSE_OUTPUT_ALPR
#define OUTPUT_ALPR

#ifndef OUTPUT_ALPR
#define printf(fmt, ...) ;
#endif


/*******************************************************************************
 * TESTS VARS
 ******************************************************************************/
static gps_point_t current_position;

static QUEUING_PORT_ID_TYPE  alpr_p_qport;
static SEMAPHORE_ID_TYPE     sync_sem;

typedef struct terrain_sample
{
    double lat;
    double lon;

    double value;
} terrain_sample_t;

static terrain_sample_t alt_array[TERRAIN_SAMPLE_SIZE];
static terrain_sample_t press_array[TERRAIN_SAMPLE_SIZE];

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

static double get_distance(double new_lat, double new_lon, double old_lat, double old_lon)
{
    double lat = TO_RAD((new_lat - old_lat));
    double lon = TO_RAD((new_lon - old_lon));

    double d_1 = sin(lat / 2) * sin(lat / 2) +
                 cos(TO_RAD(new_lat)) *
                 cos(TO_RAD(old_lat)) *
                 sin(lon / 2) * sin(lon / 2);
    double d_2 = 2 * atan2(sqrt(d_1), sqrt(1 - d_1));

    return d_2 * EARTH_RAD;
}


void* pro_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    double distance, min_distance;
    uint32_t i;
    uint32_t nearest_id;
    double current_altitude, current_pressure;

    GET_ET_STAT_1_3

    while(1)
    {
        GET_ET_STAT_2_3

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
             printf("[PRO] ERROR Cannot get partition status\n");
             return (void*)1;
        }


        WAIT_SEMAPHORE(sync_sem, INFINITE_TIME_VALUE, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[PRO] ERROR Cannot wait for semaphore [%d]\n", ret_type);
            return (void*)1;
        }

        /* Computation simulation */
        for(volatile uint32_t j = 0; j < GENERATOR_LOAD / 300; ++j)
        {
            nearest_id = 0;
            min_distance = DBL_MAX;

            /* Get nearest point for altitude */
            for(i = 0; i < TERRAIN_SAMPLE_SIZE; ++i)
            {
                /* Get distance */
                distance = get_distance(current_position.lat,
                                        current_position.lon,
                                        alt_array[i].lat,
                                        alt_array[i].lon);
                if(distance < min_distance)
                {
                    nearest_id = i;
                    min_distance = distance;
                }
            }
            current_altitude = alt_array[nearest_id].value;

            nearest_id = 0;
            min_distance = DBL_MAX;

            /* Get nearest point for pressure */
            for(i = 0; i < TERRAIN_SAMPLE_SIZE; ++i)
            {
                /* Get distance */
                distance = get_distance(current_position.lat,
                                        current_position.lon,
                                        press_array[i].lat,
                                        press_array[i].lon);
                if(distance < min_distance)
                {
                    nearest_id = i;
                    min_distance = distance;
                }
            }
            current_pressure = press_array[nearest_id].value;
        }

#ifdef VERBOSE_OUTPUT_ALPR
        printf("[p%ld:%lld|%lld|%d] COMPUTED PLANE SETTINGS: Altitude %fft, Pressure %fhPa\n",
               pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
               pr_stat.OPERATING_MODE,
               current_altitude, current_pressure);
#else
        printf("Altitude %fft, Pressure %fhPa\n",
               current_altitude, current_pressure);
#endif
        GET_ET_STAT_3_3

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[PRO] ERROR Cannot achieve periodic wait\n");
            return (void*)1;
        }
    }

    return (void*)0;
}

void* com_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;
    MESSAGE_SIZE_TYPE     msg_len;

    GET_ET_STAT_1_3

    while(1)
    {
        GET_ET_STAT_2_3

        GET_PARTITION_STATUS(&pr_stat, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot get partition status\n");
            return (void*)1;
        }


        RECEIVE_QUEUING_MESSAGE(alpr_p_qport,
                                (SYSTEM_TIME_TYPE)GPS_PERIOD,
                                (MESSAGE_ADDR_TYPE)&current_position,
                                &msg_len, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[COM] ERROR Cannot receive queueing message [%d]\n", ret_type);
        }
        else
        {
            if(msg_len != sizeof(gps_point_t))
            {
                printf("WARNING Received message has an inconsistent length\n");
            }

#ifdef VERBOSE_OUTPUT_ALPR
            printf("[p%ld:%lld|%lld|%d] RECEIVED COORDINATES: %fN %fE\n",
                   pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
                   pr_stat.OPERATING_MODE,
                   current_position.lat, current_position.lon);
#endif
            SIGNAL_SEMAPHORE(sync_sem, &ret_type);
            if(ret_type != NO_ERROR)
            {
                printf("[GEN] Cannot signal semaphore [%d]\n", ret_type);
                return (void*)1;
            }
        }

        GET_ET_STAT_3_3

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("Cannot achieve periodic wait\n");
            return (void*)1;
        }
    }

    return (void*)0;
}

/*******************************************************************************
 * TESTS MAIN
 ******************************************************************************/
int main ()
{
    PROCESS_ATTRIBUTE_TYPE tattr_com;
    PROCESS_ATTRIBUTE_TYPE tattr_pro;
    PROCESS_ID_TYPE        thread_com;
    PROCESS_ID_TYPE        thread_pro;
    RETURN_CODE_TYPE       ret_type;
    uint32_t i;
    double lat_step, lon_step;
    double current_alt, current_press;

    printf("Init ALPR partition\n");

    /* Generate terrain settings */
    lat_step = (double)(MAX_LAT - MIN_LAT) / (double)TERRAIN_SAMPLE_SIZE ;
    lon_step = (double)(MAX_LON - MIN_LON) / (double)TERRAIN_SAMPLE_SIZE;

    current_alt = 5000.0;
    current_press = 1010.0;

    srand(4003);

    for(i = 0; i < TERRAIN_SAMPLE_SIZE; ++i)
    {
        alt_array[i].lat = MIN_LAT + lat_step * i;
        alt_array[i].lon = MIN_LON + lon_step * i;
        alt_array[i].value = current_alt;
        current_alt += (double)(rand() % 30 - 10) / 10.0;

        press_array[i].lat = MIN_LAT + lat_step * i;
        press_array[i].lon = MIN_LON + lon_step * i;
        press_array[i].value = current_press;
        current_press += (double)(rand() % 3 - 1) / 10;
    }

    /* Set Communication process */
    tattr_com.ENTRY_POINT   = com_thread_func;
    tattr_com.DEADLINE      = HARD;
    tattr_com.PERIOD        = GPS_PERIOD;
    tattr_com.STACK_SIZE    = 2000;
    tattr_com.TIME_CAPACITY = 2000000000;
    tattr_com.BASE_PRIORITY = 1;

    memcpy(&tattr_com.NAME, "ALPR_COM_A653\0", 14 * sizeof(char));

    /* Set computation process */
    tattr_pro.ENTRY_POINT   = pro_thread_func;
    tattr_pro.DEADLINE      = HARD;
    tattr_pro.PERIOD        = GPS_PERIOD;
    tattr_pro.STACK_SIZE    = 2000;
    tattr_pro.TIME_CAPACITY = 2000000000;
    tattr_pro.BASE_PRIORITY = 1;

    memcpy(&tattr_pro.NAME, "ALPR_PRO_A653\0", 14 * sizeof(char));

    /* Create synchronization semaphore */
    CREATE_SEMAPHORE("ALPR_SYNC_SEM", 0, 1, FIFO, &sync_sem, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create semaphore [%d]\n", ret_type);
        return -1;
    }

    /* Create inter partition communication port */
    CREATE_QUEUING_PORT("GPS_ALPR_RECV_P_QPORT", 16, 5, DESTINATION,
                         FIFO, &alpr_p_qport, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create queing port[%d]\n", ret_type);
    }

    /* Create processes */
    CREATE_PROCESS (&tattr_com, &thread_com, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create COMMUNICATION process[%d]\n", ret_type);
    }

    CREATE_PROCESS (&tattr_pro, &thread_pro, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create PROCESSING process[%d]\n", ret_type);
    }

    /* Start all processes */
    START(thread_com, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start COMMUNICATION process[%d]\n", ret_type);
    }

    START(thread_pro, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start PROCESSING process[%d]\n", ret_type);
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("ALPR partition switching to normal mode\n");
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch ALPR partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}

#ifdef OUTPUT_ALPR
#undef printf
#endif
