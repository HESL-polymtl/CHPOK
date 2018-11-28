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
#include <string.h>
#include <libm.h>
#include <types.h>
#include <float.h>

#include <arinc653/partition.h>
#include <arinc653/types.h>
#include <arinc653/time.h>
#include <arinc653/semaphore.h>
#include <arinc653/sampling.h>

#include "../../config.h"

//#define GET_STAT 1
#include "../../../BENCH_TOOLS/get_stat.h"

/*******************************************************************************
 * TESTS SETTINGS
 ******************************************************************************/

//#define VERBOSE_OUTPUT_PROC
#define OUTPUT_PROC

#ifndef OUTPUT_PROC
#define printf(fmt, ...) ;
#endif

/*******************************************************************************
 * TESTS VARS
 ******************************************************************************/
static gps_point_t current_position;
static gps_point_t last_position = {.lat = 0.0, .lon = 0.0};

static SAMPLING_PORT_ID_TYPE  proc_p_sport;
static SEMAPHORE_ID_TYPE      sync_sem;

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


void* pro_alpr_thread_func(void)
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

#ifdef VERBOSE_OUTPUT_PROC
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

 void* pro_spdi_thread_func(void)
 {
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    double lat, lon;
    double d_1, d_2, distance;
    double speed;
    double lat1, lat2, lon1, lon2;
    double b_1, b_2, bearing;

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
   
        /* Get the distance since last point */
        lat = TO_RAD((current_position.lat - last_position.lat));
        lon = TO_RAD((current_position.lon - last_position.lon));

        d_1 = sin(lat / 2) * sin(lat / 2) +
                        cos(TO_RAD(current_position.lat)) *
                        cos(TO_RAD(last_position.lat)) *
                        sin(lon / 2) * sin(lon / 2);
        d_2 = 2 * atan2(sqrt(d_1), sqrt(1 - d_1));
        distance = d_2 * EARTH_RAD;

        /* Compute speed */
        speed = distance *  (double)HOUR / (double)GPS_PERIOD;

        /* Compute bearing */
        lat1 = TO_RAD(current_position.lat);
        lon1 = TO_RAD(current_position.lon);
        lon2 = TO_RAD(last_position.lon);
        lat2 = TO_RAD(last_position.lat);

        b_1 = sin(lon2 - lon1) * cos(lat2);
        b_2 = cos(lat2) * sin(lat2) -
                sin(lat1) * cos(lat2) * cos(lon2 - lon1);

        bearing = TO_DEG(atan2(b_1, b_2)) + 360.0;

        /* Mod */
        while(bearing >= 360.0)
        {
                bearing -= 360.0;
        }

#ifdef VERBOSE_OUTPUT_PROC
        printf("[p%ld:%lld|%lld|%d] COMPUTED PLANE SETTINGS: Dist %fKm, Speed %fKt, Bearing %fdeg\n",
               pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
               pr_stat.OPERATING_MODE,
               distance, speed * 0.539957, bearing);
#else
        printf("Distance %fKm, Speed %fKt, Bearing %fdeg\n",
               distance, speed * 0.539957, bearing);
#endif

        last_position = current_position;

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
    VALIDITY_TYPE         msg_validity;

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


        READ_SAMPLING_MESSAGE(proc_p_sport,
                              (MESSAGE_ADDR_TYPE)&current_position,
                              &msg_len, &msg_validity, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[COM] ERROR Cannot receive sampling message [%d]\n", ret_type);
        }
        else
        {
            if(msg_len != sizeof(gps_point_t))
            {
                printf("WARNING Received message has an inconsistent length\n");
            }
            if(msg_validity != VALID)
            {
                printf("WARNING Received message has is marked as invalid\n");

            }

#ifdef VERBOSE_OUTPUT_PROC
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
    PROCESS_ATTRIBUTE_TYPE tattr_pro_spdi;
    PROCESS_ATTRIBUTE_TYPE tattr_pro_alpr;
    PROCESS_ID_TYPE        thread_com;
    PROCESS_ID_TYPE        thread_pro_spdi;
    PROCESS_ID_TYPE        thread_pro_alpr;
    RETURN_CODE_TYPE       ret_type;

    uint32_t i;
    double lat_step, lon_step;
    double current_alt, current_press;

    srand(4003);

    /* Generate terrain settings */
    lat_step = (double)(MAX_LAT - MIN_LAT) / (double)TERRAIN_SAMPLE_SIZE ;
    lon_step = (double)(MAX_LON - MIN_LON) / (double)TERRAIN_SAMPLE_SIZE;

    current_alt = 5000.0;
    current_press = 1010.0;

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

    printf("Init PROC partition\n");

    /* Set Communication process */
    tattr_com.ENTRY_POINT   = com_thread_func;
    tattr_com.DEADLINE      = HARD;
    tattr_com.PERIOD        = GPS_PERIOD;
    tattr_com.STACK_SIZE    = 2000;
    tattr_com.TIME_CAPACITY = 5000000000;
    tattr_com.BASE_PRIORITY = 1;

    memcpy(&tattr_com.NAME, "PROC_COM_A653\0", 14 * sizeof(char));

    /* Set spdi computation process */
    tattr_pro_spdi.ENTRY_POINT   = pro_spdi_thread_func;
    tattr_pro_spdi.DEADLINE      = HARD;
    tattr_pro_spdi.PERIOD        = GPS_PERIOD;
    tattr_pro_spdi.STACK_SIZE    = 2000;
    tattr_pro_spdi.TIME_CAPACITY = 3750000000;
    tattr_pro_spdi.BASE_PRIORITY = 1;

    memcpy(&tattr_pro_spdi.NAME, "PROC_SPDI_A653\0", 15 * sizeof(char));

    /* Set altr computation process */
    tattr_pro_alpr.ENTRY_POINT   = pro_alpr_thread_func;
    tattr_pro_alpr.DEADLINE      = HARD;
    tattr_pro_alpr.PERIOD        = GPS_PERIOD;
    tattr_pro_alpr.STACK_SIZE    = 2000;
    tattr_pro_alpr.TIME_CAPACITY = 3750000000;
    tattr_pro_alpr.BASE_PRIORITY = 5;

    memcpy(&tattr_pro_alpr.NAME, "PROC_ALPR_A653\0", 15 * sizeof(char));

    /* Create synchronization semaphore */
    CREATE_SEMAPHORE("PROC_SYNC_SEM", 0, 2, FIFO, &sync_sem, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create semaphore [%d]\n", ret_type);
        return -1;
    }

    /* Create inter partition communication port */
    CREATE_SAMPLING_PORT("GPS_PROC_RECV_P_SPORT", 16, DESTINATION,
                         GPS_PERIOD, &proc_p_sport, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create sampling port[%d]\n", ret_type);
    }

    /* Create processes */
    CREATE_PROCESS (&tattr_com, &thread_com, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create COMMUNICATION process[%d]\n", ret_type);
    }

    CREATE_PROCESS (&tattr_pro_spdi, &thread_pro_spdi, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create SPDI PROCESSING process[%d]\n", ret_type);
    }

    CREATE_PROCESS (&tattr_pro_alpr, &thread_pro_alpr, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create ALPR PROCESSING process[%d]\n", ret_type);
    }

    /* Start all processes */
    START(thread_com, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start COMMUNICATION process[%d]\n", ret_type);
    }

    START(thread_pro_spdi, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start SPDI PROCESSING process[%d]\n", ret_type);
    }

    START(thread_pro_alpr, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start ALPR PROCESSING process[%d]\n", ret_type);
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("PROC partition switching to normal mode\n");
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch PROC partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}

#ifdef OUTPUT_PROC
#undef printf
#endif
