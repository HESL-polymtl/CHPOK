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

#ifndef __GPS_NAV_CONFIG_H_
#define __GPS_NAV_CONFIG_H_

#define GPS_PERIOD    2000000000 /* GPS period in ms */
#define EARTH_RAD     6378.1     /* Earth Radius in Km */

#define MIN_LAT   40.044000
#define MIN_LON    6.050000
#define MAX_LAT   40.055000
#define MAX_LON    6.068000

#define START_LAT (MIN_LAT + 0.005948)
#define START_LON (MIN_LON + 0.001901)

#define TERRAIN_SAMPLE_SIZE 100

#define SECOND        1000000000    /* 1s */
#define MINUTE        60000000000   /* 1min */
#define HOUR          3600000000000 /* 1hour */

#define M_PI 3.14159265358979323846

#define TO_RAD(x) (M_PI / 180.0 * x)
#define TO_DEG(x) (180.0 / M_PI * x)

typedef struct gps_point
{
    double lat;
    double lon;
} gps_point_t;

#endif /* __GPS_NAV_CONFIG_H_ */
