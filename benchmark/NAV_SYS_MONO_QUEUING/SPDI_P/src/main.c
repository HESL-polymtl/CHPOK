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
//#define VERBOSE_OUTPUT_SPDI
#define OUTPUT_SPDI

#ifndef OUTPUT_SPDI
#define printf(fmt, ...) ;
#endif

/*******************************************************************************
 * TESTS VARS
 ******************************************************************************/
static gps_point_t current_position;
static gps_point_t last_position = {.lat = 0.0, .lon = 0.0};

static QUEUING_PORT_ID_TYPE spdi_p_qport;
static SEMAPHORE_ID_TYPE      sync_sem;
/*******************************************************************************
 * TESTS FUNCTIONS
 ******************************************************************************/

 void* pro_thread_func(void)
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

        /* Computation simulation */
        for(volatile uint32_t j = 0; j < GENERATOR_LOAD; ++j)
        {
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
        }

#ifdef VERBOSE_OUTPUT_ALPR
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


        RECEIVE_QUEUING_MESSAGE(spdi_p_qport, (SYSTEM_TIME_TYPE)GPS_PERIOD,
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

    printf("Init SPDI partition\n");

    /* Set Communication process */
    tattr_com.ENTRY_POINT   = com_thread_func;
    tattr_com.DEADLINE      = HARD;
    tattr_com.PERIOD        = GPS_PERIOD;
    tattr_com.STACK_SIZE    = 2000;
    tattr_com.TIME_CAPACITY = 2000000000;
    tattr_com.BASE_PRIORITY = 1;

    memcpy(&tattr_com.NAME, "SPDI_COM_A653\0", 14 * sizeof(char));

    /* Set computation process */
    tattr_pro.ENTRY_POINT   = pro_thread_func;
    tattr_pro.DEADLINE      = HARD;
    tattr_pro.PERIOD        = GPS_PERIOD;
    tattr_pro.STACK_SIZE    = 2000;
    tattr_pro.TIME_CAPACITY = 2000000000;
    tattr_pro.BASE_PRIORITY = 1;

    memcpy(&tattr_pro.NAME, "SPDI_PRO_A653\0", 14 * sizeof(char));

    /* Create synchronization semaphore */
    CREATE_SEMAPHORE("SPDI_SYNC_SEM", 0, 1, FIFO, &sync_sem, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create semaphore [%d]\n", ret_type);
        return -1;
    }

    /* Create inter partition communication port */
    CREATE_QUEUING_PORT("GPS_SPDI_RECV_P_QPORT", 16, 5, DESTINATION,
                        FIFO, &spdi_p_qport, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot create queuing port[%d]\n", ret_type);
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
    printf("SPDI partition switching to normal mode\n");
    SET_PARTITION_MODE (NORMAL, &ret_type);
    if(ret_type != NO_ERROR)
    {
        printf("Cannot switch SPDI partition to NORMAL state[%d]\n", ret_type);
        return -1;
    }

    STOP_SELF();

    return (0);
}

#ifdef OUTPUT_SPDI
#undef printf
#endif
