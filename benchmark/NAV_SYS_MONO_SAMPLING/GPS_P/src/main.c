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

#define GENERATOR_LOAD 10000  /* Processing simulation */

#define PLANE_SPEED   210.0      /* Plane speed in Km/H */
#define PLANE_BEARING 10         /* Plane bearing in deg
                                    (must be a divisor of 360)*/

#define OUTPUT_GPS
//#define VERBOSE_OUTPUT_GPS

#ifndef OUTPUT_GPS
#define printf(fmt, ...) ;
#endif
/*******************************************************************************
 * TESTS VARS
 ******************************************************************************/

static gps_point_t current_position;

static SEMAPHORE_ID_TYPE      sync_sem;
static SAMPLING_PORT_ID_TYPE  gps_spdi_p_sport;
static SAMPLING_PORT_ID_TYPE  gps_alpr_p_sport;

/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

/* Asin of POK mathlib is returning wrong values, TODO maybe we can check that
 * and fix POK mathlib.
 *
 * So here is an aproximation
 */
double asin(double x)
{
    return atan2(x, sqrt((1.0 + x) * (1.0 - x)));
}

void* com_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint8_t error;


    GET_ET_STAT_1_3

    while(1)
    {
        error = 0;
        GET_ET_STAT_2_3

        /* When the DATA should be ready to be sent */
        WAIT_SEMAPHORE(sync_sem, INFINITE_TIME_VALUE, &ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[COM] ERROR Cannot wait for semaphore [%d]\n", ret_type);
            return (void*)1;
        }


        WRITE_SAMPLING_MESSAGE(gps_spdi_p_sport,
                               (MESSAGE_ADDR_TYPE)&current_position,
                               sizeof(current_position),
                               &ret_type);
        if(ret_type != NO_ERROR)
        {
            error = 1;
            printf("[COM] ERROR Cannot send sampling message [%d]\n", ret_type);
        }

        WRITE_SAMPLING_MESSAGE(gps_alpr_p_sport,
                               (MESSAGE_ADDR_TYPE)&current_position,
                               sizeof(current_position),
                               &ret_type);
        if(ret_type != NO_ERROR)
        {
            error = 1;
            printf("[COM] ERROR Cannot send sampling message [%d]\n", ret_type);
        }

        if(error == 0)
        {
#ifdef VERBOSE_OUTPUT_GPS
            printf("[p%ld:%lld|%lld|%d] DATA SENT\n",
                   pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
                   pr_stat.OPERATING_MODE);
#endif
        }

        GET_ET_STAT_3_3

        PERIODIC_WAIT(&ret_type);
        if(ret_type != NO_ERROR)
        {
            printf("[COM] Cannot achieve periodic wait [%d]\n", ret_type);
            return (void*)1;
        }
    }

    return NULL;
}

void* gen_thread_func(void)
{
    RETURN_CODE_TYPE      ret_type;
    PARTITION_STATUS_TYPE pr_stat;

    uint32_t i;

    double lat_part1, lat_part2;
    double lon_part1, lon_part2;
    double new_lat, new_lon;
    double rad_lat, rad_lon;

    double dist_since_last;
    uint32_t init_brng;
    uint32_t step;

    GET_ET_STAT_1_3

    while(1)
    {

        GET_ET_STAT_2_3

        init_brng = PLANE_BEARING;

        current_position.lat = START_LAT;
        current_position.lon = 6.0519;

        step = 360 / PLANE_BEARING;

        /* Circle pattern:  */
        for(i = 0; i < step; ++i)
        {

            if(i > 0)
            {
                GET_ET_STAT_2_3
            }

            /* Computation simulation */
            for(volatile uint32_t j = 0; j < GENERATOR_LOAD; ++j)
            {

                /* Get new distance */
                dist_since_last = ((GPS_PERIOD / 1000000000.0) * PLANE_SPEED) /
                                  3600.0;

                rad_lat = TO_RAD(current_position.lat);
                rad_lon = TO_RAD(current_position.lon);


                /* Generate new point */
                lat_part1 = sin(rad_lat) *
                            cos(dist_since_last / EARTH_RAD);
                lat_part2 = rad_lat *
                            sin(dist_since_last / EARTH_RAD) *
                            cos(TO_RAD((double)init_brng));

                new_lat = asin(lat_part1 + lat_part2);

                lon_part1 = sin(TO_RAD((double)init_brng)) *
                            sin(dist_since_last / EARTH_RAD) *
                            cos(rad_lat);
                lon_part2 = cos(dist_since_last / EARTH_RAD) -
                            sin(rad_lat) *
                            sin(new_lat);

                new_lon = rad_lon +
                          atan2(lon_part1, lon_part2);
            }

            /* Set new point */
            current_position.lat = TO_DEG(new_lat);
            current_position.lon = TO_DEG(new_lon);

            init_brng = (init_brng + PLANE_BEARING) % 360;

            GET_PARTITION_STATUS(&pr_stat, &ret_type);
            if(ret_type != NO_ERROR)
            {
                printf("[GEN] Cannot get partition status [%d]\n", ret_type);
                return (void*)1;
            }
#ifdef VERBOSE_OUTPUT_GPS
            printf("[p%ld:%lld|%lld|%d] NEW POINT GENERATED, %fN %fE\n",
                   pr_stat.IDENTIFIER, pr_stat.PERIOD, pr_stat.DURATION,
                   pr_stat.OPERATING_MODE, current_position.lat, current_position.lon);
#else
            printf("NEW POINT GENERATED, %fN %fE\n",
                   current_position.lat, current_position.lon);
#endif
            SIGNAL_SEMAPHORE(sync_sem, &ret_type);
            if(ret_type != NO_ERROR)
            {
                printf("[GEN] Cannot signal semaphore [%d]\n", ret_type);
                return (void*)1;
            }

            GET_ET_STAT_3_3

            PERIODIC_WAIT(&ret_type);
            if(ret_type != NO_ERROR)
            {
                printf("[GEN] Cannot achieve periodic wait [%d]\n", ret_type);
                return (void*)1;
            }
       }
    }

    return (void*)0;
}

/*******************************************************************************
 * TESTS MAIN
 ******************************************************************************/
int main()
{
    PROCESS_ID_TYPE        thread_gen;
    PROCESS_ID_TYPE        thread_com;
    PROCESS_ATTRIBUTE_TYPE tattr_gen;
    PROCESS_ATTRIBUTE_TYPE tattr_com;

    RETURN_CODE_TYPE       ret_type;

    /* Set GENERATOR process */
    tattr_gen.ENTRY_POINT   = gen_thread_func;
    tattr_gen.DEADLINE      = HARD;
    tattr_gen.PERIOD        = GPS_PERIOD;
    tattr_gen.STACK_SIZE    = 2000;
    tattr_gen.TIME_CAPACITY = 2000000000;
    tattr_gen.BASE_PRIORITY = 25;
    memcpy(tattr_gen.NAME, "GPS_GEN_A653\0", 13 * sizeof(char));

    /* Set COMMUNICATION process */
    tattr_com.ENTRY_POINT   = com_thread_func;
    tattr_com.DEADLINE      = HARD;
    tattr_com.PERIOD        = GPS_PERIOD;
    tattr_com.STACK_SIZE    = 2000;
    tattr_com.TIME_CAPACITY = 2000000000;
    tattr_com.BASE_PRIORITY = 50;
    memcpy(tattr_com.NAME, "GPS_COM_A653\0", 13 * sizeof(char));

    printf("Init GPS partition\n");

    /* Create synchronization semaphore */
    CREATE_SEMAPHORE("GPS_SYNC_SEM", 0, 1, FIFO, &sync_sem, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create semaphore [%d]\n", ret_type);
        return -1;
    }

    /* Create inter partition communication port (sampling mode) */
    CREATE_SAMPLING_PORT("GPS_SPDI_SEND_P_SPORT", 16, SOURCE,
                         GPS_PERIOD, &gps_spdi_p_sport, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create sampling port [%d]\n", ret_type);
        return -1;
    }
    CREATE_SAMPLING_PORT("GPS_ALPR_SEND_P_SPORT", 16, SOURCE,
                         GPS_PERIOD, &gps_alpr_p_sport, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create sampling port [%d]\n", ret_type);
        return -1;
    }

    /* Create processes */
    CREATE_PROCESS(&tattr_gen, &thread_gen, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create GENEATOR process [%d]\n", ret_type);
        return -1;
    }
    CREATE_PROCESS(&tattr_com, &thread_com, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create COMMUNICATION process [%d]\n", ret_type);
        return -1;
    }

    /* Start all processes */
    START(thread_gen, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start GENERATOR process[%d]\n", ret_type);
        return -1;
    }
    START(thread_com, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot start COMMUNICATION process[%d]\n", ret_type);
        return -1;
    }

    /* Parition has been initialized, now switch to normal mode */
    printf("GPS partition switching to normal mode\n");
    SET_PARTITION_MODE(NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch GPS partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return 0;
}

#ifdef OUTPUT_GPS
#undef printf
#endif
